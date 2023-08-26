using SimHub.Plugins.OutputPlugins.Dash.GLCDTemplating;
using System;
using System.Collections.Generic;
using System.IO.Ports;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Controls;

using System.Windows.Media.TextFormatting;
using System.Text.Json;
using FMOD;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Json;
using System.IO;
using System.Text;
using System.Web;
using MahApps.Metro.Controls;
using System.Runtime.CompilerServices;
using System.CodeDom.Compiler;
using System.Windows.Forms;
using static System.Net.Mime.MediaTypeNames;
using System.Runtime.InteropServices.ComTypes;
using Microsoft.Win32;
using System.Xml.Linq;


namespace User.PluginSdkDemo
{


    /// <summary>
    /// Logique d'interaction pour SettingsControlDemo.xaml
    /// </summary>
    public partial class SettingsControlDemo : System.Windows.Controls.UserControl
    {


        public uint pedalConfigPayload_version = 105;

        public uint indexOfSelectedPedal_u = 1;

        public DataPluginDemo Plugin { get; }

        public DAP_config_st[] dap_config_st = new DAP_config_st[3];
        private string stringValue;
        private unsafe byte* loadedConfig_p;







        // read config from JSON on startup
        //ReadStructFromJson();


        // read JSON config from JSON file
        //private void ReadStructFromJson()
        //{



        //    try
        //    {
        //        // https://learn.microsoft.com/en-us/dotnet/standard/serialization/system-text-json/how-to?pivots=dotnet-8-0
        //        // https://www.educative.io/answers/how-to-read-a-json-file-in-c-sharp

        //        string currentDirectory = Directory.GetCurrentDirectory();
        //        string dirName = currentDirectory + "\\PluginsData\\Common";
        //        //string jsonFileName = ComboBox_JsonFileSelected.Text;
        //        string jsonFileName = ((ComboBoxItem)ComboBox_JsonFileSelected.SelectedItem).Content.ToString();
        //        string fileName = dirName + "\\" + jsonFileName + ".json";

        //        string text = System.IO.File.ReadAllText(fileName);

        //        DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(DAP_config_st));
        //        var ms = new MemoryStream(Encoding.UTF8.GetBytes(text));
        //        dap_config_st[indexOfSelectedPedal_u] = (DAP_config_st)deserializer.ReadObject(ms);
        //        //TextBox1.Text = "Config loaded!";
        //        //TextBox1.Text += ComboBox_JsonFileSelected.Text;
        //        //TextBox1.Text += "    ";
        //        //TextBox1.Text += ComboBox_JsonFileSelected.SelectedIndex;

        //        updateTheGuiFromConfig();

        //    }
        //    catch (Exception caughtEx)
        //    {

        //        string errorMessage = caughtEx.Message;
        //        TextBox1.Text = errorMessage;
        //    }


        //}

        private void InitReadStructFromJson()
        {



            try
            {
                // https://learn.microsoft.com/en-us/dotnet/standard/serialization/system-text-json/how-to?pivots=dotnet-8-0
                // https://www.educative.io/answers/how-to-read-a-json-file-in-c-sharp
                string jsonFileName="NA";

                string currentDirectory = Directory.GetCurrentDirectory();
                string dirName = currentDirectory + "\\PluginsData\\Common";
                //string jsonFileName = ComboBox_JsonFileSelected.Text;
                if (indexOfSelectedPedal_u == 0)
                {
                    jsonFileName = ("DiyPedalConfig_Clutch_Default");
                }
                else if (indexOfSelectedPedal_u == 1)
                {
                    jsonFileName = ("DiyPedalConfig_Brake_Default");
                }
                else if (indexOfSelectedPedal_u == 2)
                {
                     jsonFileName = ("DiyPedalConfig_Accelerator_Default");                    
                }

                string fileName = dirName + "\\" + jsonFileName + ".json";
                string text = System.IO.File.ReadAllText(fileName);



                DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(DAP_config_st));
                var ms = new MemoryStream(Encoding.UTF8.GetBytes(text));
                dap_config_st[indexOfSelectedPedal_u] = (DAP_config_st)deserializer.ReadObject(ms);
                TextBox1.Text = "Config loaded!";
                //TextBox1.Text += ComboBox_JsonFileSelected.Text;
                //TextBox1.Text += "    ";
                //TextBox1.Text += ComboBox_JsonFileSelected.SelectedIndex;

                updateTheGuiFromConfig();

            }
            catch (Exception caughtEx)
            {

                string errorMessage = caughtEx.Message;
                TextBox1.Text = errorMessage;
            }


        }

        private void UpdateSerialPortList_click()
        {

            var SerialPortSelectionArray = new List<SerialPortChoice>();
            string[] comPorts = SerialPort.GetPortNames();
            if (comPorts.Length > 0)
            {

                foreach (string portName in comPorts)
                {
                    SerialPortSelectionArray.Add(new SerialPortChoice(portName, portName));
                }
                TextBox1.Text = "Serial port List updated";
            }
            else
            {
                SerialPortSelectionArray.Add(new SerialPortChoice("NA", "NA"));
                TextBox1.Text = "No Serial port found";
            }
            
            SerialPortSelection.DataContext = SerialPortSelectionArray;
        }



        public SettingsControlDemo()
        {

            for (uint pedalIdx = 0; pedalIdx < 3; pedalIdx++)
            {
                dap_config_st[pedalIdx].payloadHeader_.payloadType = 100;
                dap_config_st[pedalIdx].payloadHeader_.version = (byte)pedalConfigPayload_version;

                dap_config_st[pedalIdx].payloadPedalConfig_.pedalStartPosition = 35;
                dap_config_st[pedalIdx].payloadPedalConfig_.pedalEndPosition = 80;
                dap_config_st[pedalIdx].payloadPedalConfig_.maxForce = 90;
                dap_config_st[pedalIdx].payloadPedalConfig_.relativeForce_p000 = 0;
                dap_config_st[pedalIdx].payloadPedalConfig_.relativeForce_p020 = 20;
                dap_config_st[pedalIdx].payloadPedalConfig_.relativeForce_p040 = 40;
                dap_config_st[pedalIdx].payloadPedalConfig_.relativeForce_p060 = 60;
                dap_config_st[pedalIdx].payloadPedalConfig_.relativeForce_p080 = 80;
                dap_config_st[pedalIdx].payloadPedalConfig_.relativeForce_p100 = 100;
                dap_config_st[pedalIdx].payloadPedalConfig_.dampingPress = 0;
                dap_config_st[pedalIdx].payloadPedalConfig_.dampingPull = 0;
                dap_config_st[pedalIdx].payloadPedalConfig_.absFrequency = 5;
                dap_config_st[pedalIdx].payloadPedalConfig_.absAmplitude = 20;
                dap_config_st[pedalIdx].payloadPedalConfig_.lengthPedal_AC = 150;
                dap_config_st[pedalIdx].payloadPedalConfig_.horPos_AB = 215;
                dap_config_st[pedalIdx].payloadPedalConfig_.verPos_AB = 80;
                dap_config_st[pedalIdx].payloadPedalConfig_.lengthPedal_CB = 200;


                dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.cubic_spline_param_a_0 = 0;
                dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.cubic_spline_param_a_1 = 0;
                dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.cubic_spline_param_a_2 = 0;
                dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.cubic_spline_param_a_3 = 0;
                dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.cubic_spline_param_a_4 = 0;

                dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.cubic_spline_param_b_0 = 0;
                dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.cubic_spline_param_b_1 = 0;
                dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.cubic_spline_param_b_2 = 0;
                dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.cubic_spline_param_b_3 = 0;
                dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.cubic_spline_param_b_4 = 0;

                dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.PID_p_gain = 0.3f;
                dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.PID_i_gain = 50.0f;
                dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.PID_d_gain = 0.0f;

                InitializeComponent();

            }

        }



        public byte[] getBytesPayload(payloadPedalConfig aux)
        {
            int length = Marshal.SizeOf(aux);
            IntPtr ptr = Marshal.AllocHGlobal(length);
            byte[] myBuffer = new byte[length];

            Marshal.StructureToPtr(aux, ptr, true);
            Marshal.Copy(ptr, myBuffer, 0, length);
            Marshal.FreeHGlobal(ptr);

            return myBuffer;
        }


        public byte[] getBytes(DAP_config_st aux)
        {
            int length = Marshal.SizeOf(aux);
            IntPtr ptr = Marshal.AllocHGlobal(length);
            byte[] myBuffer = new byte[length];

            Marshal.StructureToPtr(aux, ptr, true);
            Marshal.Copy(ptr, myBuffer, 0, length);
            Marshal.FreeHGlobal(ptr);

            return myBuffer;
        }


        unsafe private UInt16 checksumCalc(byte* data, int length)
        {

            UInt16 curr_crc = 0x0000;
            byte sum1 = (byte)curr_crc;
            byte sum2 = (byte)(curr_crc >> 8);
            int index;
            for (index = 0; index < length; index = index + 1)
            {
                int v = (sum1 + (*data));
                sum1 = (byte)v;
                sum1 = (byte)(v % 255);

                int w = (sum1 + sum2) % 255;
                sum2 = (byte)w;

                data++;// = data++;
            }

            int x = (sum2 << 8) | sum1;
            return (UInt16)x;
        }


        public SettingsControlDemo(DataPluginDemo plugin) : this()
        {
            this.Plugin = plugin;

            UpdateSerialPortList_click();

            // check if Json config files are present, otherwise create new ones
            //for (int jsonIndex = 0; jsonIndex < ComboBox_JsonFileSelected.Items.Count; jsonIndex++)
            //{

            //    ComboBox_JsonFileSelected.SelectedIndex = jsonIndex;

            //    // which config file is seleced
            //    string currentDirectory = Directory.GetCurrentDirectory();
            //    string dirName = currentDirectory + "\\PluginsData\\Common";
            //    //string jsonFileName = ComboBox_JsonFileSelected(ComboBox_JsonFileSelected.Items[jsonIndex]).Text;
            //    string jsonFileName = ((ComboBoxItem)ComboBox_JsonFileSelected.SelectedItem).Content.ToString();
            //    string fileName = dirName + "\\" + jsonFileName + ".json";


            //    // Check if file already exists, otherwise create    
            //    if (!File.Exists(fileName))
            //    {
            //        // create default config
            //        // https://stackoverflow.com/questions/3275863/does-net-4-have-a-built-in-json-serializer-deserializer
            //        // https://learn.microsoft.com/en-us/dotnet/framework/wcf/feature-details/how-to-serialize-and-deserialize-json-data?redirectedfrom=MSDN
            //        var stream1 = new MemoryStream();
            //        var ser = new DataContractJsonSerializer(typeof(DAP_config_st));
            //        ser.WriteObject(stream1, Plugin.dap_config_initial_st);

            //        stream1.Position = 0;
            //        StreamReader sr = new StreamReader(stream1);
            //        string jsonString = sr.ReadToEnd();

            //        System.IO.File.WriteAllText(fileName, jsonString);
            //    }
            //}

            string currentDirectory = Directory.GetCurrentDirectory();
            string dirName = currentDirectory + "\\PluginsData\\Common";
            //string jsonFileName = ComboBox_JsonFileSelected(ComboBox_JsonFileSelected.Items[jsonIndex]).Text;
            string jsonFileNameA = "DiyPedalConfig_Accelerator_Default";
            string jsonFileNameB = "DiyPedalConfig_Brake_Default";
            string jsonFileNameC = "DiyPedalConfig_Clutch_Default";
            string fileNameA = dirName + "\\" + jsonFileNameA + ".json";
            string fileNameB = dirName + "\\" + jsonFileNameB + ".json";
            string fileNameC = dirName + "\\" + jsonFileNameC + ".json";

            if (!File.Exists(fileNameA))
            {
                // create default config
                // https://stackoverflow.com/questions/3275863/does-net-4-have-a-built-in-json-serializer-deserializer
                // https://learn.microsoft.com/en-us/dotnet/framework/wcf/feature-details/how-to-serialize-and-deserialize-json-data?redirectedfrom=MSDN
                var stream1 = new MemoryStream();
                var ser = new DataContractJsonSerializer(typeof(DAP_config_st));
                ser.WriteObject(stream1, Plugin.dap_config_initial_st);

                stream1.Position = 0;
                StreamReader sr = new StreamReader(stream1);
                string jsonString = sr.ReadToEnd();

                System.IO.File.WriteAllText(fileNameA, jsonString);
            }

            if (!File.Exists(fileNameB))
            {
                // create default config
                // https://stackoverflow.com/questions/3275863/does-net-4-have-a-built-in-json-serializer-deserializer
                // https://learn.microsoft.com/en-us/dotnet/framework/wcf/feature-details/how-to-serialize-and-deserialize-json-data?redirectedfrom=MSDN
                var stream1 = new MemoryStream();
                var ser = new DataContractJsonSerializer(typeof(DAP_config_st));
                ser.WriteObject(stream1, Plugin.dap_config_initial_st);

                stream1.Position = 0;
                StreamReader sr = new StreamReader(stream1);
                string jsonString = sr.ReadToEnd();

                System.IO.File.WriteAllText(fileNameB, jsonString);
            }
            if (!File.Exists(fileNameC))
            {
                // create default config
                // https://stackoverflow.com/questions/3275863/does-net-4-have-a-built-in-json-serializer-deserializer
                // https://learn.microsoft.com/en-us/dotnet/framework/wcf/feature-details/how-to-serialize-and-deserialize-json-data?redirectedfrom=MSDN
                var stream1 = new MemoryStream();
                var ser = new DataContractJsonSerializer(typeof(DAP_config_st));
                ser.WriteObject(stream1, Plugin.dap_config_initial_st);

                stream1.Position = 0;
                StreamReader sr = new StreamReader(stream1);
                string jsonString = sr.ReadToEnd();

                System.IO.File.WriteAllText(fileNameC, jsonString);
            }

            for (uint pedalIndex = 0; pedalIndex < 3; pedalIndex++)
            {
                indexOfSelectedPedal_u = pedalIndex;
                //ComboBox_JsonFileSelected.SelectedIndex = Plugin.Settings.selectedJsonFileNames[indexOfSelectedPedal_u];
                //ComboBox_JsonFileSelected.SelectedIndex = Plugin.Settings.selectedJsonIndexLast[indexOfSelectedPedal_u];
                InitReadStructFromJson();
                updateTheGuiFromConfig();

            }

        }




        private void updateTheGuiFromConfig()
        {
            // update the sliders
            PedalMinForce_Slider.Value = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.preloadForce;
            PedalMaxForce_Slider.Value = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.maxForce;

            PedalMinPos_Slider.Value = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.pedalStartPosition;
            PedalMaxPos_Slider.Value = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.pedalEndPosition;

            PedalAbsAmplitude_Slider.Value = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.absAmplitude;
            PedalAbsFrequency_Slider.Value = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.absFrequency;

            PedalDampening_Slider.Value = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.dampingPress;

            PedalForceCurve000_Slider.Value = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p000;
            PedalForceCurve020_Slider.Value = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p020;
            PedalForceCurve040_Slider.Value = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p040;
            PedalForceCurve060_Slider.Value = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p060;
            PedalForceCurve080_Slider.Value = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p080;
            PedalForceCurve100_Slider.Value = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p100;


            PID_tuning_P_gain_slider.Value = (double)dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.PID_p_gain;
            PID_tuning_I_gain_slider.Value = (double)dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.PID_i_gain;
            PID_tuning_D_gain_slider.Value = (double)dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.PID_d_gain;


            maxGameOutput_slider.Value = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.maxGameOutput;

            Update_BrakeForceCurve();


            //// Select serial port accordingly
            string tmp = (string)Plugin._serialPort[indexOfSelectedPedal_u].PortName;
            if (!string.Equals(tmp,"NA") && tmp != null)
            {
                try
                {
                    SerialPortSelection.SelectedValue = tmp;
                    TextBox1.Text += "\rSerial port selected: " + SerialPortSelection.SelectedValue;

                }
                catch (Exception caughtEx)
                {
                }
            }
            
            //else
            //{
                
            //    TextBox1.Text += "\rNo Serial port found";
            //}
            


            if (Plugin._serialPort[indexOfSelectedPedal_u].IsOpen == true)
            {
                ConnectToPedal.IsChecked = true;
            }
            else
            {
                ConnectToPedal.IsChecked = false;
            }

            //try
            //{
            //    //ComboBox_JsonFileSelected.SelectedItem = Plugin.Settings.selectedJsonFileNames[indexOfSelectedPedal_u];
            //    //ComboBox_JsonFileSelected.SelectedValue = (string)Plugin.Settings.selectedJsonFileNames[indexOfSelectedPedal_u];

            //    ComboBox_JsonFileSelected.SelectedIndex = Plugin.Settings.selectedJsonIndexLast[indexOfSelectedPedal_u];

            //    //ReadStructFromJson();


            //    //SerialPortSelection.SelectedValue
            //    //TextBox1.Text = "Error 2: ";
            //    //TextBox1.Text += Plugin.Settings.selectedJsonFileNames[indexOfSelectedPedal_u];
            //    //TextBox1.Text += "     ";
            //    //TextBox1.Text += ComboBox_JsonFileSelected.SelectedValue;
            //}
            //catch (Exception caughtEx)
            //{
            //    string errorMessage = caughtEx.Message;
            //    TextBox1.Text = "Error 1: ";
            //    TextBox1.Text += errorMessage;
            //}

            //= ComboBox_JsonFileSelected.SelectedItem.ToString();

            //ConnectToPedal.IsChecked = true;

            //TextBox1.Text = "Pedal selected: " + indexOfSelectedPedal_u;
            //TextBox1.Text += ",    connected: " + ConnectToPedal.IsChecked;
            //TextBox1.Text += ",    serial port name: " + tmp;

        }




        private void Update_BrakeForceCurve()
        {

            double[] x = new double[6];
            double[] y = new double[6];
            x[0] = 0;
            x[1] = 20;
            x[2] = 40;
            x[3] = 60;
            x[4] = 80;
            x[5] = 100;

            y[0] = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p000;
            y[1] = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p020;
            y[2] = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p040;
            y[3] = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p060;
            y[4] = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p080;
            y[5] = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p100;

            // Use cubic interpolation to smooth the original data
            (double[] xs2, double[] ys2, double[] a, double[] b) = Cubic.Interpolate1D(x, y, 100);


            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.cubic_spline_param_a_0 = (float)a[0];
            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.cubic_spline_param_a_1 = (float)a[1];
            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.cubic_spline_param_a_2 = (float)a[2];
            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.cubic_spline_param_a_3 = (float)a[3];
            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.cubic_spline_param_a_4 = (float)a[4];

            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.cubic_spline_param_b_0 = (float)b[0];
            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.cubic_spline_param_b_1 = (float)b[1];
            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.cubic_spline_param_b_2 = (float)b[2];
            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.cubic_spline_param_b_3 = (float)b[3];
            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.cubic_spline_param_b_4 = (float)b[4];


            //TextBox1.Text = "";
            //for (uint i = 0; i < a.Length; i++)
            //{
            //    TextBox1.Text += "\na[" + i + "]: " + a[i] + "      b[" + i + "]: " + b[i];
            //}


            System.Windows.Media.PointCollection myPointCollection2 = new System.Windows.Media.PointCollection();


            for (int pointIdx = 0; pointIdx < 100; pointIdx++)
            {
                System.Windows.Point Pointlcl = new System.Windows.Point(4 * xs2[pointIdx], ys2[pointIdx]);
                myPointCollection2.Add(Pointlcl);
            }

            this.Polyline_BrakeForceCurve.Points = myPointCollection2;

        }



        public class SerialPortChoice
        {
            public SerialPortChoice(string display, string value)
            {
                Display = display;
                Value = value;
            }

            public string Value { get; set; }
            public string Display { get; set; }
        }



        // Select which pedal to config
        // see https://stackoverflow.com/questions/772841/is-there-selected-tab-changed-event-in-the-standard-wpf-tab-control
        private void TabControl_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            indexOfSelectedPedal_u = (uint)MyTab.SelectedIndex;
            TextBox1.Text = "";
            // update the sliders & serial port selection accordingly
            updateTheGuiFromConfig();
        }





        /********************************************************************************************************************/
        /*							Slider callbacks																		*/
        /********************************************************************************************************************/
        private void Slider_PedalMinForce(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.preloadForce = Convert.ToByte(e.NewValue);

            if (dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.preloadForce > dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.maxForce)
            {
                dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.preloadForce = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.maxForce;
                PedalMinForce_Slider.Value = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.preloadForce;
            }
        }

        private void Slider_PedalMaxForce(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.maxForce = Convert.ToByte(e.NewValue);

            if (dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.maxForce < dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.preloadForce)
            {
                dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.maxForce = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.preloadForce;
                PedalMaxForce_Slider.Value = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.maxForce;
            }
        }


        private void Slider_PedalMinPos(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.pedalStartPosition = Convert.ToByte(e.NewValue);

            if (dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.pedalStartPosition > dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.pedalEndPosition)
            {
                dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.pedalStartPosition = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.pedalEndPosition;
                PedalMinPos_Slider.Value = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.pedalStartPosition;
            }
        }

        private void Slider_PedalMaxPos(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.pedalEndPosition = Convert.ToByte(e.NewValue);

            if (dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.pedalEndPosition < dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.pedalStartPosition)
            {
                dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.pedalEndPosition = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.pedalStartPosition;
                PedalMaxPos_Slider.Value = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.pedalEndPosition;
            }
        }

        private void Slider_AbsAmplitude(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.absAmplitude = Convert.ToByte(e.NewValue);
        }

        private void Slider_AbsFrequency(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.absFrequency = Convert.ToByte(e.NewValue);
        }

        public void TestAbs_click(object sender, RoutedEventArgs e)
        {
            //if (indexOfSelectedPedal_u == 1)
                if (TestAbs.IsChecked==false)
                { 
                    TestAbs.IsChecked= true;
                    Plugin.sendAbsSignal = (bool)TestAbs.IsChecked;
                    TextBox1.Text = "ABS-Test begin";
                }
                else
                {
                    TestAbs.IsChecked = false;
                    //Plugin.sendAbsSignal = !Plugin.sendAbsSignal;
                    Plugin.sendAbsSignal = (bool)TestAbs.IsChecked;
                    TextBox1.Text = "ABS-Test stopped";
                }
        }

        public void Slider_Dampening(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.dampingPress = Convert.ToByte(e.NewValue);
            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.dampingPull = Convert.ToByte(e.NewValue);
        }




        public void Slider_Force000(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p000 = Convert.ToByte(e.NewValue);
            Update_BrakeForceCurve();
        }
        public void Slider_Force020(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p020 = Convert.ToByte(e.NewValue);
            Update_BrakeForceCurve();
        }
        public void Slider_Force040(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p040 = Convert.ToByte(e.NewValue);
            Update_BrakeForceCurve();
        }
        public void Slider_Force060(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p060 = Convert.ToByte(e.NewValue);
            Update_BrakeForceCurve();
        }
        public void Slider_Force080(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p080 = Convert.ToByte(e.NewValue);
            Update_BrakeForceCurve();
        }
        public void Slider_Force100(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p100 = Convert.ToByte(e.NewValue);
            Update_BrakeForceCurve();
        }



        /********************************************************************************************************************/
        /*							PID tuning                      														*/
        /********************************************************************************************************************/
        public void PID_tuning_P_gain_changed(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.PID_p_gain = (float)e.NewValue;
        }

        public void PID_tuning_I_gain_changed(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.PID_i_gain = (float)e.NewValue;
        }

        public void PID_tuning_D_gain_changed(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.PID_d_gain = (float)e.NewValue;
        }


        public void maxGameOutput_changed(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.maxGameOutput = (byte)e.NewValue;
        }


        /********************************************************************************************************************/
        /*							Write/read config to/from Json file														*/
        /********************************************************************************************************************/

        //private void ComboBox_SelectionChanged(object sender, EventArgs e)
        //{
        //    try
        //    {
        //        // https://stackoverflow.com/questions/3721430/what-is-the-simplest-way-to-get-the-selected-text-of-a-combo-box-containing-only

        //        string stringValue = ((ComboBoxItem)ComboBox_JsonFileSelected.SelectedItem).Content.ToString();


        //        // string stringValue = ComboBox_JsonFileSelected.SelectedValue.ToString();

        //        //TextBox1.Text = stringValue;
        //        Plugin.Settings.selectedJsonFileNames[indexOfSelectedPedal_u] = stringValue;

        //        Plugin.Settings.selectedJsonIndexLast[indexOfSelectedPedal_u] = ComboBox_JsonFileSelected.SelectedIndex;



        //        //ReadStructFromJson();
        //    }
        //    catch (Exception caughtEx)
        //    {

        //        string errorMessage = caughtEx.Message;
        //        TextBox1.Text = errorMessage;
        //    }
        //}




        //public void SaveStructToJson_click(object sender, RoutedEventArgs e)
        //{
        //    // https://learn.microsoft.com/en-us/dotnet/standard/serialization/system-text-json/how-to?pivots=dotnet-8-0

        //    try
        //    {
        //        // which config file is seleced
        //        string currentDirectory = Directory.GetCurrentDirectory();
        //        string dirName = currentDirectory + "\\PluginsData\\Common";
        //        string jsonFileName = ComboBox_JsonFileSelected.Text;
        //        string fileName = dirName + "\\" + jsonFileName + ".json";

        //        this.dap_config_st[indexOfSelectedPedal_u].payloadHeader_.version = (byte)pedalConfigPayload_version;

        //        // https://stackoverflow.com/questions/3275863/does-net-4-have-a-built-in-json-serializer-deserializer
        //        // https://learn.microsoft.com/en-us/dotnet/framework/wcf/feature-details/how-to-serialize-and-deserialize-json-data?redirectedfrom=MSDN
        //        var stream1 = new MemoryStream();
        //        var ser = new DataContractJsonSerializer(typeof(DAP_config_st));
        //        ser.WriteObject(stream1, dap_config_st[indexOfSelectedPedal_u]);

        //        stream1.Position = 0;
        //        StreamReader sr = new StreamReader(stream1);
        //        string jsonString = sr.ReadToEnd();

        //        // Check if file already exists. If yes, delete it.     
        //        if (File.Exists(fileName))
        //        {
        //            File.Delete(fileName);
        //        }


        //        System.IO.File.WriteAllText(fileName, jsonString);
        //        TextBox1.Text = "Config exported!";

        //    }
        //    catch (Exception caughtEx)
        //    {

        //        string errorMessage = caughtEx.Message;
        //        TextBox1.Text = errorMessage;
        //    }

        //}



        //public void ReadStructFromJson_click(object sender, RoutedEventArgs e)
        //{
        //    ReadStructFromJson();
        //}


        /********************************************************************************************************************/
        /*							Refind min endstop																		*/
        /********************************************************************************************************************/
        public void ResetPedalPosition_click(object sender, RoutedEventArgs e)
        {

            if (Plugin._serialPort[indexOfSelectedPedal_u].IsOpen)
            {

                if (Plugin._serialPort[indexOfSelectedPedal_u].BytesToRead > 0)
                {
                    Plugin._serialPort[indexOfSelectedPedal_u].DiscardInBuffer();
                }

                try
                {
                    Plugin._serialPort[indexOfSelectedPedal_u].Write("1");
                    int myInt = 1;
                    byte[] b = BitConverter.GetBytes(myInt);
                    Plugin._serialPort[indexOfSelectedPedal_u].Write(b, 0, 4);
                    Plugin._serialPort[indexOfSelectedPedal_u].Write("\n");
                    TextBox1.Text = "Reset Pedal position";
                    System.Threading.Thread.Sleep(100);
                }
                catch (Exception caughtEx)
                {
                    string errorMessage = caughtEx.Message;
                    TextBox1.Text = errorMessage;
                }




                try
                {
                    while (Plugin._serialPort[indexOfSelectedPedal_u].BytesToRead > 0)
                    {
                        string message = Plugin._serialPort[indexOfSelectedPedal_u].ReadLine();
                    }
                }
                catch (TimeoutException) { }
            }
            else
            {
                TextBox1.Text = "Serial port not open";
            }
        }



        /********************************************************************************************************************/
        /*							Refind min endstop																		*/
        /********************************************************************************************************************/
        public void StartSystemIdentification_click(object sender, RoutedEventArgs e)
        {

            TextBox1.Text = "Start system identification";


            try
            {

                string currentDirectory = Directory.GetCurrentDirectory();
                string dirName = currentDirectory + "\\PluginsData\\Common";
                string logFileName = "DiyActivePedalSystemIdentification";
                string fileName = dirName + "\\" + logFileName + ".txt";

                if (File.Exists(fileName))
                {
                    File.Delete(fileName);
                }


                // This text is added only once to the file.
                if (!File.Exists(fileName))
                {
                    using (StreamWriter sw = File.CreateText(fileName))
                    {

                        // trigger system identification
                        Plugin._serialPort[indexOfSelectedPedal_u].Write("3");

                        System.Threading.Thread.Sleep(100);


                        // read system return log
                        while (Plugin._serialPort[indexOfSelectedPedal_u].BytesToRead > 0)
                        {
                            string message = Plugin._serialPort[indexOfSelectedPedal_u].ReadLine();
                            sw.Write(message);

                            System.Threading.Thread.Sleep(20);

                        }
                    }

                }

                TextBox1.Text = "Finished system identification";


                ////// trigger system identification
                ////Plugin._serialPort[indexOfSelectedPedal_u].Write("3");

                ////System.Threading.Thread.Sleep(100);


                ////// read system return log
                ////while (Plugin._serialPort[indexOfSelectedPedal_u].BytesToRead > 0)
                ////{
                ////    string message = Plugin._serialPort[indexOfSelectedPedal_u].ReadLine();
                ////    using (StreamWriter sw = File.AppendText(fileName))
                ////    {
                ////        sw.WriteLine(message);
                ////    }
                ////    System.Threading.Thread.Sleep(100);
                ////}


            }
            catch (Exception caughtEx)
            {
                string errorMessage = caughtEx.Message;
                TextBox1.Text = errorMessage;
            }

        }


        /********************************************************************************************************************/
        /*							Send config to pedal																	*/
        /********************************************************************************************************************/
        unsafe public void SendConfigToPedal_click(object sender, RoutedEventArgs e)
        {
            if (Plugin._serialPort[indexOfSelectedPedal_u].IsOpen)
            {

                // compute checksum
                //getBytes(this.dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_)
                payloadPedalConfig tmp = this.dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_;
                payloadPedalConfig* v = &tmp;
                byte* p = (byte*)v;
                this.dap_config_st[indexOfSelectedPedal_u].payloadHeader_.checkSum = checksumCalc(p, sizeof(payloadPedalConfig));

                TextBox1.Text = "Send Config to Pedal\r";
                TextBox1.Text += "CRC simhub calc: " + this.dap_config_st[indexOfSelectedPedal_u].payloadHeader_.checkSum + "\r";


                try
                {
                    this.dap_config_st[indexOfSelectedPedal_u].payloadHeader_.version = (byte)pedalConfigPayload_version;
                    this.dap_config_st[indexOfSelectedPedal_u].payloadHeader_.storeToEeprom = true;
                    int length = sizeof(DAP_config_st);
                    int val = this.dap_config_st[indexOfSelectedPedal_u].payloadHeader_.checkSum;
                    string msg = "CRC value: " + val.ToString();
                    byte[] newBuffer = new byte[length];
                    newBuffer = getBytes(this.dap_config_st[indexOfSelectedPedal_u]);
                    Plugin._serialPort[indexOfSelectedPedal_u].Write(newBuffer, 0, newBuffer.Length);
                    //Plugin._serialPort[indexOfSelectedPedal_u].Write("\n");
                }
                catch (Exception caughtEx)
                {
                    string errorMessage = caughtEx.Message;
                    TextBox1.Text = errorMessage;
                }

                System.Threading.Thread.Sleep(100);
                try
                {
                    while (Plugin._serialPort[indexOfSelectedPedal_u].BytesToRead > 0)
                    {
                        string message = Plugin._serialPort[indexOfSelectedPedal_u].ReadLine();

                        TextBox1.Text += "Pedal --> " + message;
                    }
                }
                catch (TimeoutException) { }



            }
            else
            {
                TextBox1.Text = "Serial port not open";
            }
        }





        /********************************************************************************************************************/
        /*							Connect to pedal																		*/
        /********************************************************************************************************************/
        public void ConnectToPedal_click(object sender, RoutedEventArgs e)
        {


            if (ConnectToPedal.IsChecked == false)
            {
                if (Plugin._serialPort[indexOfSelectedPedal_u].IsOpen == false)
                {
                    try
                    {
                        Plugin._serialPort[indexOfSelectedPedal_u].Open();
                        TextBox1.Text = "Serial port "+ Plugin._serialPort[indexOfSelectedPedal_u].PortName+" open";
                        ConnectToPedal.IsChecked = true;

                        try
                        {
                            while (Plugin._serialPort[indexOfSelectedPedal_u].BytesToRead > 0)
                            {
                                string message = Plugin._serialPort[indexOfSelectedPedal_u].ReadLine();                                
                            }
                        }
                        catch (TimeoutException) { }

                    }
                    catch (Exception ex)
                    {
                        TextBox1.Text = ex.Message;
                        ConnectToPedal.IsChecked = false;
                    }

                }
                else
                {
                    //Plugin._serialPort[indexOfSelectedPedal_u].Close();
                    //ConnectToPedal.IsChecked = false;
                    TextBox1.Text = "Serial port already open";
                }
            }

        }

        /********************************************************************************************************************/
        /*							Serial port selection																	*/
        /********************************************************************************************************************/
        public void UpdateSerialPortList_click(object sender, RoutedEventArgs e)
        {
            UpdateSerialPortList_click();
        }

        public void SerialPortSelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            string tmp = (string)SerialPortSelection.SelectedValue;
            //Plugin._serialPort[indexOfSelectedPedal_u].PortName = tmp;


            //try 
            //{
            //    TextBox1.Text = "Debug: " + Plugin.Settings.selectedComPortNames[indexOfSelectedPedal_u];
            //}
            //catch (Exception caughtEx)
            //{
            //    string errorMessage = caughtEx.Message;
            //    TextBox1.Text = errorMessage;
            //}
            if (!string.Equals(tmp, "NA") && tmp != null)
            {
                    try
                    {
                        Plugin.Settings.selectedComPortNames[indexOfSelectedPedal_u] = tmp;
                        Plugin._serialPort[indexOfSelectedPedal_u].PortName = tmp;
                        Plugin._serialPort[indexOfSelectedPedal_u].ReadTimeout = 2000;
                        Plugin._serialPort[indexOfSelectedPedal_u].WriteTimeout = 500;


                        TextBox1.Text = "Serial port selected: " + Plugin.Settings.selectedComPortNames[indexOfSelectedPedal_u];
                        
                }
                    catch (Exception caughtEx)
                    {
                        string errorMessage = caughtEx.Message;
                        TextBox1.Text = errorMessage;
                    }
                }
            
            else
            {
                TextBox1.Text = "No Serial port found";
            }

        }
        private void OpenButton_Click(object sender, EventArgs e)
        {
            using (System.Windows.Forms.OpenFileDialog openFileDialog = new System.Windows.Forms.OpenFileDialog())
            {
                openFileDialog.Title = "Datei auswählen";
                openFileDialog.Filter = "Configdateien (*.json)|*.json";
                string currentDirectory = Directory.GetCurrentDirectory();
                openFileDialog.InitialDirectory = currentDirectory + "\\PluginsData\\Common";

                if (openFileDialog.ShowDialog() == DialogResult.OK)
                {

                    string filePath = openFileDialog.FileName;
                    string text1 = System.IO.File.ReadAllText(filePath);
                    string content = (string)openFileDialog.FileName;
                    TextBox1.Text = content;
                    DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(DAP_config_st));
                    var ms = new MemoryStream(Encoding.UTF8.GetBytes(text1));
                    dap_config_st[indexOfSelectedPedal_u] = (DAP_config_st)deserializer.ReadObject(ms);
                    //TextBox1.Text = "Config loaded!";
                    //TextBox1.Text += ComboBox_JsonFileSelected.Text;
                    //TextBox1.Text += "    ";
                    //TextBox1.Text += ComboBox_JsonFileSelected.SelectedIndex;
                    updateTheGuiFromConfig();
                    TextBox1.Text = "Config new imported!";
                    TextBox2.Text = "Open "+openFileDialog.FileName;
                }
            }

        }

        private void SaveButton_Click(object sender, RoutedEventArgs e)
        {
            using (System.Windows.Forms.SaveFileDialog saveFileDialog = new System.Windows.Forms.SaveFileDialog())
            {
                saveFileDialog.Title = "Datei speichern";
                saveFileDialog.Filter = "Textdateien (*.json)|*.json";
                string currentDirectory = Directory.GetCurrentDirectory();
                saveFileDialog.InitialDirectory = currentDirectory + "\\PluginsData\\Common";

                if (saveFileDialog.ShowDialog() == DialogResult.OK)
                {
                     string fileName = saveFileDialog.FileName;
                

                this.dap_config_st[indexOfSelectedPedal_u].payloadHeader_.version = (byte)pedalConfigPayload_version;

                // https://stackoverflow.com/questions/3275863/does-net-4-have-a-built-in-json-serializer-deserializer
                // https://learn.microsoft.com/en-us/dotnet/framework/wcf/feature-details/how-to-serialize-and-deserialize-json-data?redirectedfrom=MSDN
                var stream1 = new MemoryStream();
                var ser = new DataContractJsonSerializer(typeof(DAP_config_st));
                ser.WriteObject(stream1, dap_config_st[indexOfSelectedPedal_u]);

                stream1.Position = 0;
                StreamReader sr = new StreamReader(stream1);
                string jsonString = sr.ReadToEnd();

                // Check if file already exists. If yes, delete it.     
                if (File.Exists(fileName))
                {
                    File.Delete(fileName);
                }


                System.IO.File.WriteAllText(fileName, jsonString);
                TextBox1.Text = "Config exported!";
                TextBox2.Text = "Save " + saveFileDialog.FileName;
                }
            }
        }

        private void DisconnectToPedal_click(object sender, RoutedEventArgs e)
        {
            if (ConnectToPedal.IsChecked == true)
            {
                Plugin._serialPort[indexOfSelectedPedal_u].Close();
                ConnectToPedal.IsChecked = false;
                TextBox1.Text = "Serial port "+ Plugin._serialPort[indexOfSelectedPedal_u].PortName+" close";
            }           
            else
            {
                ConnectToPedal.IsChecked = false;
                Plugin._serialPort[indexOfSelectedPedal_u].Close();
                TextBox1.Text = "Not Checked Serial port close";
            }

        }

        unsafe public void LoadConfigfromPedal_click(object sender, RoutedEventArgs e)
        {
            if (Plugin._serialPort[indexOfSelectedPedal_u].IsOpen)
            {
                Plugin._serialPort[indexOfSelectedPedal_u].Write("4");
                TextBox1.Text = "Load Config from Pedal\r";
                System.Threading.Thread.Sleep(1000);
                try
                {
                    while (Plugin._serialPort[indexOfSelectedPedal_u].BytesToRead > 0)
                    {
                        string message = Plugin._serialPort[indexOfSelectedPedal_u].ReadLine();
                        if (string.Equals(message, "Send Config\r"))
                        { 
                            TextBox1.Text += "Pedal --> " + message;
                            System.Threading.Thread.Sleep(500);
                        }
                        else if (string.Equals(message, "Default case:\r"))
                        {
                            TextBox1.Text += "Ende   " + message+"\r";
                            
                            //System.Threading.Thread.Sleep(100);
                        }
                        else 
                        {
                            TextBox1.Text += "Data   " + message + "\r";

                            //string currentDirectory = Directory.GetCurrentDirectory();
                            //string dirName = currentDirectory + "\\PluginsData\\Common";
                            //string jsonFileName = "DiyPedalConfig_TMP";                           
                            //string fileName = dirName + "\\" + jsonFileName + ".json";
                            //System.IO.File.WriteAllText(fileName, message);

                            var jsonObject = JsonDocument.Parse(message).RootElement;
                            var pedalConfig = jsonObject.GetProperty("payloadPedalConfig_");

                            // Convert the JSON payloadPedalConfig_ to bytes
                            var loadedConfig = Encoding.UTF8.GetBytes(pedalConfig.ToString());
                            fixed (byte* loadedConfig_p = loadedConfig)
                            {
                                int loadedConfiglenght = loadedConfig.Length;
                                ushort a = checksumCalc(loadedConfig_p, loadedConfiglenght);
                                TextBox1.Text += "calculated CRC = " + a + "\r";
                            
                                var headerConfig = jsonObject.GetProperty("payloadHeader_");
                                var check1 = headerConfig.TryGetProperty("checkSum", out JsonElement value);
                                TextBox1.Text +="Pedal CRC = "+ value.ToString() + "\r";
                                
                            }   
                            

                            //DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(DAP_config_st));
                            //var ms = new MemoryStream(Encoding.UTF8.GetBytes(message));
                            //dap_config_st[indexOfSelectedPedal_u] = (DAP_config_st)deserializer.ReadObject(ms);

                            ////TextBox1.Text = "Config loaded!";
                            ////TextBox1.Text += ComboBox_JsonFileSelected.Text;
                            ////TextBox1.Text += "    ";
                            ////TextBox1.Text += ComboBox_JsonFileSelected.SelectedIndex;
                            TextBox1.Text += "Loaded Config from Pedal";
                            //updateTheGuiFromConfig();
                            
                        }                        
                    }
                }
                catch (TimeoutException) { }                
            }
            else
            { 
                TextBox1.Text = "Serial port not open";
            }
        }
    }
    
}
