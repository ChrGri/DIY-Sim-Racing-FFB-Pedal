using System;
using System.Diagnostics;
using System.IO;
using System.Net.Http;
using System.Reflection;
using System.Threading.Tasks;

namespace DiyFfbPedal.UIFunction
{
    // Updates the plugin in two steps: the files are downloaded while SimHub keeps running, then a small
    // cmd script waits for SimHub to exit, swaps them in and starts SimHub again.
    internal static class PluginUpdateHelper
    {
        private static readonly string TempDir = Path.Combine(Path.GetTempPath(), "DiyFfbPedal_update");
        private static bool downloading;

        // Throws if the plugin cannot be downloaded or is not a valid plugin DLL. Nothing has been changed
        // at that point. The .resx is optional: if it cannot be downloaded, the current one is kept.
        public static async Task DownloadAsync(string dllUrl, string resxUrl)
        {
            if (downloading)
            {
                throw new InvalidOperationException("An update is already being downloaded.");
            }
            downloading = true;
            try
            {
                string dllPath = Path.Combine(TempDir, "DiyFfbPedal.dll");
                string resxPath = Path.Combine(TempDir, "DiyFfbPedal.resx");
                Directory.CreateDirectory(TempDir);
                File.Delete(dllPath);
                File.Delete(resxPath);
                using (HttpClient client = new HttpClient { Timeout = TimeSpan.FromMinutes(10) })
                {
                    File.WriteAllBytes(dllPath, await client.GetByteArrayAsync(dllUrl));
                    // Never install something that is not the plugin, e.g. an error page or a truncated file.
                    bool valid;
                    try
                    {
                        valid = AssemblyName.GetAssemblyName(dllPath).Name == "DiyFfbPedal";
                    }
                    catch (Exception)
                    {
                        valid = false;
                    }
                    if (!valid)
                    {
                        File.Delete(dllPath);
                        throw new InvalidDataException("The downloaded file is not a valid DiyFfbPedal plugin.");
                    }

                    if (resxUrl != null)
                    {
                        try
                        {
                            File.WriteAllBytes(resxPath, await client.GetByteArrayAsync(resxUrl));
                        }
                        catch (Exception)
                        {
                            File.Delete(resxPath);
                        }
                    }
                }
            }
            finally
            {
                downloading = false;
            }
        }

        // Starts the script with admin rights (needed for the SimHub folder) and closes SimHub.
        // Throws if the script cannot be started, e.g. when the admin prompt is declined.
        public static void InstallAndRestart()
        {
            string simhubDir = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
            if (!File.Exists(Path.Combine(simhubDir, "SimHubWPF.exe")))
            {
                throw new FileNotFoundException("SimHubWPF.exe was not found next to the plugin.");
            }
            if (!File.Exists(Path.Combine(TempDir, "DiyFfbPedal.dll")))
            {
                throw new FileNotFoundException("The update has not been downloaded.");
            }

            string scriptPath = Path.Combine(TempDir, "update.cmd");
            // %1 = SimHub folder, %2 = folder with the downloaded files. The backup must succeed before the
            // plugin is replaced, and a failed copy puts the backup back, so SimHub always has a working plugin.
            File.WriteAllLines(scriptPath, new[]
            {
                "@echo off",
                "title DIY FFB Pedal plugin update",
                "echo Waiting for SimHub to close...",
                "set n=0",
                ":wait",
                @"tasklist /fi ""IMAGENAME eq SimHubWPF.exe"" 2>nul | find /i ""SimHubWPF.exe"" >nul || goto closed",
                "set /a n+=1",
                "if %n% geq 60 taskkill /f /im SimHubWPF.exe >nul 2>&1",
                "timeout /t 1 /nobreak >nul 2>&1 || ping -n 2 127.0.0.1 >nul",
                "goto wait",
                ":closed",
                "echo Installing plugin...",
                @"copy /y ""%~1\DiyFfbPedal.dll"" ""%~1\DiyFfbPedal.dll.bak"" >nul || goto failed",
                @"copy /y ""%~2\DiyFfbPedal.dll"" ""%~1\DiyFfbPedal.dll"" >nul || goto restore",
                @"if exist ""%~2\DiyFfbPedal.resx"" if not exist ""%~1\languages"" mkdir ""%~1\languages""",
                @"if exist ""%~2\DiyFfbPedal.resx"" copy /y ""%~2\DiyFfbPedal.resx"" ""%~1\languages\DiyFfbPedal.resx"" >nul",
                @"del ""%~2\DiyFfbPedal.dll"" ""%~2\DiyFfbPedal.resx"" >nul 2>&1",
                "echo Update completed successfully! Restarting SimHub...",
                "timeout /t 2 /nobreak >nul 2>&1",
                // /d: SimHub and the plugin resolve their data folders from the working directory, and an
                // elevated cmd starts in System32.
                @"start """" /d ""%~1"" ""%~1\SimHubWPF.exe""",
                "exit /b 0",
                ":restore",
                @"copy /y ""%~1\DiyFfbPedal.dll.bak"" ""%~1\DiyFfbPedal.dll"" >nul",
                ":failed",
                "echo The plugin could not be replaced. The previous version was kept.",
                "timeout /t 10 /nobreak 2>nul || ping -n 11 127.0.0.1 >nul",
                @"start """" /d ""%~1"" ""%~1\SimHubWPF.exe""",
            });
            Process.Start(new ProcessStartInfo
            {
                FileName = "cmd.exe",
                Arguments = $"/c \"\"{scriptPath}\" \"{simhubDir}\" \"{TempDir}\"\"",
                Verb = "runas",
                UseShellExecute = true
            });
            System.Windows.Application.Current.Shutdown();
        }
    }
}
