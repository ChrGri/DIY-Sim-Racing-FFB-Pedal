using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text.RegularExpressions;

namespace DiyFfbPedal
{
    // JavaScript for script effects, run by the Jint engine that ships with SimHub
    // (the same engine SimHub uses for its own JavaScript formulas).
    //
    // Jint is reached by reflection at runtime, so the plugin neither ships its own
    // copy nor depends on the Jint version of the installed SimHub (Jint 2 and 3+
    // differ in their API).
    //
    // Script: the body of a function(t, dt, prev, phase). Use "return x;" like in
    // SimHub's JavaScript formulas; a single expression without return works too.
    // Available: $prop("Property.Name"), root (object kept between calls), pi, Math.
    //
    // Not thread safe; the effect pipeline engine only uses it under its lock.
    internal sealed class JintScriptHost
    {
        // a script that runs longer is stopped (an endless loop would block SimHub)
        private const int TimeoutMs = 20;
        private const int MaxStatements = 100000;
        private const int RecursionLimit = 64;

        private const string MainFunction = "__diyffb_main";
        private const string FreqFunction = "__diyffb_freq";

        private static readonly object InitLock = new object();
        private static bool _initialized;
        private static string _loadError;
        private static Type _optionsType;
        private static ConstructorInfo _ctorWithOptions;
        private static MethodInfo _execute;
        private static MethodInfo _setValue;
        private static MethodInfo _getValue;
        private static readonly Dictionary<Type, MethodInfo> ToObjectMethods = new Dictionary<Type, MethodInfo>();
        private static List<MethodInfo> _optionExtensions;
        [ThreadStatic] private static bool _timeoutApplied;

        private readonly object _engine;
        private readonly bool _hasFreq;
        private readonly object[] _oneArg = new object[1];
        private readonly object[] _twoArgs = new object[2];

        // Throws with a readable message when Jint is missing or the script has a syntax error.
        public JintScriptHost(string mainBody, string freqBody, Func<string, object> prop)
        {
            EnsureLoaded();

            _timeoutApplied = false;
            Type actionType = typeof(Action<>).MakeGenericType(_optionsType);
            MethodInfo configure = typeof(JintScriptHost)
                .GetMethod("ConfigureOptions", BindingFlags.NonPublic | BindingFlags.Static)
                .MakeGenericMethod(_optionsType);
            Delegate configureDelegate = Delegate.CreateDelegate(actionType, configure);
            _engine = Unwrap(() => _ctorWithOptions.Invoke(new object[] { configureDelegate }));
            if (!_timeoutApplied)
                throw new InvalidOperationException("JavaScript not available: the Jint version of this SimHub has no script timeout");

            SetValue("$prop", prop);
            _hasFreq = !string.IsNullOrWhiteSpace(freqBody);
            string source =
                "var root = {};\n" +
                "var pi = Math.PI;\n" +
                "function " + MainFunction + "(t, dt, prev, phase) {\n" + ToFunctionBody(mainBody) + "\n}\n" +
                (_hasFreq ? "function " + FreqFunction + "(t, dt, prev, phase) {\n" + ToFunctionBody(freqBody) + "\n}\n" : "");
            Execute(source);
        }

        public bool HasFrequency { get { return _hasFreq; } }

        public static bool IsAvailable(out string error)
        {
            try
            {
                EnsureLoaded();
                error = null;
                return true;
            }
            catch (Exception ex)
            {
                error = ex.Message;
                return false;
            }
        }

        public double CallMain(double t, double dt, double prev, double phase)
        {
            return Call(MainFunction, t, dt, prev, phase);
        }

        public double CallFrequency(double t, double dt, double prev, double phase)
        {
            return _hasFreq ? Call(FreqFunction, t, dt, prev, phase) : 0.0;
        }

        private double Call(string function, double t, double dt, double prev, double phase)
        {
            SetValue("__t", t);
            SetValue("__dt", dt);
            SetValue("__prev", prev);
            SetValue("__phase", phase);
            // Execute (not Invoke) so that Jint restarts the timeout and statement counter
            Execute("__r = " + function + "(__t, __dt, __prev, __phase);");
            return ToDouble(GetValue("__r"));
        }

        // "return" anywhere: a full function body; otherwise a single expression
        private static string ToFunctionBody(string body)
        {
            body = body ?? "";
            if (Regex.IsMatch(body, @"\breturn\b")) return body;
            string expression = body.Trim().TrimEnd(';').Trim();
            if (expression.Length == 0) return "return 0;";
            return "return (" + expression + "\n);";
        }

        private void Execute(string source)
        {
            ParameterInfo[] ps = _execute.GetParameters();
            object[] args = new object[ps.Length];
            args[0] = source;
            for (int i = 1; i < ps.Length; i++) args[i] = ps[i].HasDefaultValue ? ps[i].DefaultValue : Type.Missing;
            Unwrap(() => _execute.Invoke(_engine, args));
        }

        private void SetValue(string name, object value)
        {
            _twoArgs[0] = name;
            _twoArgs[1] = value;
            Unwrap(() => _setValue.Invoke(_engine, _twoArgs));
        }

        private object GetValue(string name)
        {
            _oneArg[0] = name;
            object jsValue = Unwrap(() => _getValue.Invoke(_engine, _oneArg));
            if (jsValue == null) return null;
            MethodInfo toObject;
            Type type = jsValue.GetType();
            if (!ToObjectMethods.TryGetValue(type, out toObject))
            {
                toObject = type.GetMethod("ToObject", Type.EmptyTypes);
                ToObjectMethods[type] = toObject;
            }
            return toObject != null ? Unwrap(() => toObject.Invoke(jsValue, null)) : null;
        }

        private static double ToDouble(object value)
        {
            if (value == null) return 0.0; // undefined / null
            if (value is bool b) return b ? 1.0 : 0.0;
            if (value is string s)
            {
                double parsed;
                return double.TryParse(s, NumberStyles.Float, CultureInfo.InvariantCulture, out parsed) ? parsed : 0.0;
            }
            try { return Convert.ToDouble(value, CultureInfo.InvariantCulture); }
            catch { throw new InvalidOperationException("script returned " + value.GetType().Name + ", expected a number"); }
        }

        private static object Unwrap(Func<object> call)
        {
            try
            {
                return call();
            }
            catch (TargetInvocationException ex) when (ex.InnerException != null)
            {
                Exception inner = ex.InnerException;
                if (inner is TimeoutException || inner.GetType().Name.Contains("Timeout"))
                    throw new InvalidOperationException("script took longer than " + TimeoutMs + " ms and was stopped");
                if (inner.GetType().Name.Contains("StatementsCount"))
                    throw new InvalidOperationException("script ran more than " + MaxStatements + " statements and was stopped");
                throw new InvalidOperationException(inner.Message, inner);
            }
        }

        // Action<Options> handed to the Engine constructor. Jint 2 has these as Options
        // methods, Jint 3+ as extension methods; both are tried.
        private static void ConfigureOptions<T>(T options)
        {
            if (ApplyOption(options, "TimeoutInterval", TimeSpan.FromMilliseconds(TimeoutMs)))
                _timeoutApplied = true;
            ApplyOption(options, "MaxStatements", MaxStatements);
            ApplyOption(options, "LimitRecursion", RecursionLimit);
        }

        private static bool ApplyOption(object options, string name, object argument)
        {
            try
            {
                MethodInfo instance = _optionsType.GetMethod(name, BindingFlags.Public | BindingFlags.Instance,
                                                             null, new[] { argument.GetType() }, null);
                if (instance != null)
                {
                    instance.Invoke(options, new[] { argument });
                    return true;
                }
                foreach (MethodInfo m in _optionExtensions)
                {
                    if (m.Name != name) continue;
                    ParameterInfo[] ps = m.GetParameters();
                    if (ps.Length == 2 && ps[1].ParameterType == argument.GetType())
                    {
                        m.Invoke(null, new[] { options, argument });
                        return true;
                    }
                }
            }
            catch (Exception ex)
            {
                SimHub.Logging.Current.Warn("Script effects: Jint option " + name + " failed: " + ex.Message);
            }
            return false;
        }

        private static void EnsureLoaded()
        {
            lock (InitLock)
            {
                if (!_initialized)
                {
                    _initialized = true;
                    try
                    {
                        Load();
                    }
                    catch (Exception ex)
                    {
                        _loadError = "JavaScript not available: " + ex.Message;
                        SimHub.Logging.Current.Warn("Script effects: " + _loadError);
                    }
                }
                if (_loadError != null) throw new InvalidOperationException(_loadError);
            }
        }

        private static void Load()
        {
            Assembly jint = AppDomain.CurrentDomain.GetAssemblies().FirstOrDefault(a => a.GetName().Name == "Jint");
            if (jint == null)
            {
                string path = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "Jint.dll");
                if (!File.Exists(path)) throw new FileNotFoundException("Jint.dll not found in the SimHub folder");
                jint = Assembly.LoadFrom(path);
            }

            Type engineType = jint.GetType("Jint.Engine", true);
            _optionsType = jint.GetType("Jint.Options", true);
            _ctorWithOptions = engineType.GetConstructor(new[] { typeof(Action<>).MakeGenericType(_optionsType) });
            if (_ctorWithOptions == null) throw new MissingMethodException("Jint.Engine(Action<Options>)");

            _execute = engineType.GetMethods(BindingFlags.Public | BindingFlags.Instance)
                .Where(m => m.Name == "Execute")
                .Where(m =>
                {
                    ParameterInfo[] ps = m.GetParameters();
                    return ps.Length >= 1 && ps[0].ParameterType == typeof(string) && ps.Skip(1).All(p => p.IsOptional);
                })
                .OrderBy(m => m.GetParameters().Length)
                .FirstOrDefault();
            if (_execute == null) throw new MissingMethodException("Jint.Engine.Execute(string)");

            _setValue = engineType.GetMethod("SetValue", new[] { typeof(string), typeof(object) });
            if (_setValue == null) throw new MissingMethodException("Jint.Engine.SetValue(string, object)");
            _getValue = engineType.GetMethod("GetValue", new[] { typeof(string) });
            if (_getValue == null) throw new MissingMethodException("Jint.Engine.GetValue(string)");

            Type[] types;
            try { types = jint.GetTypes(); }
            catch (ReflectionTypeLoadException ex) { types = ex.Types.Where(t => t != null).ToArray(); }
            _optionExtensions = types
                .Where(t => t.IsAbstract && t.IsSealed) // static classes
                .SelectMany(t => t.GetMethods(BindingFlags.Public | BindingFlags.Static))
                .Where(m =>
                {
                    ParameterInfo[] ps = m.GetParameters();
                    return ps.Length == 2 && ps[0].ParameterType == _optionsType;
                })
                .ToList();

            SimHub.Logging.Current.Info("Script effects: using Jint " + jint.GetName().Version + " for JavaScript");
        }
    }
}
