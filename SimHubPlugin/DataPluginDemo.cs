using GameReaderCommon;
using SimHub.Plugins;
using SimHub.Plugins.OutputPlugins.Dash.GLCDTemplating;
using System;
using System.IO.Ports;
using System.Runtime;
using System.Runtime.InteropServices;
using System.Windows.Media;




// https://stackoverflow.com/questions/14344305/best-way-to-structure-class-struct-in-c-sharp
[StructLayout(LayoutKind.Sequential, Pack = 1)]
[Serializable]





public struct payloadHeader
{
    // structure identification via payload
    public byte payloadType;

    // variable to check if structure at receiver matched version from transmitter
    public byte version;

    // To check if structure is valid
    public UInt16 checkSum;

    public bool storeToEeprom;
}
public struct payloadPedalConfig
{
    // configure pedal start and endpoint
    // In percent
    public byte pedalStartPosition;
    public byte pedalEndPosition;

    // configure pedal forces
    public byte maxForce;
    public byte preloadForce;

    // design force vs travel curve
    // In percent
    public byte relativeForce_p000;
    public byte relativeForce_p020;
    public byte relativeForce_p040;
    public byte relativeForce_p060;
    public byte relativeForce_p080;
    public byte relativeForce_p100;

    // parameter to configure damping
    public byte dampingPress;
    public byte dampingPull;

    // configure ABS effect 
    public byte absFrequency; // In Hz
    public byte absAmplitude; // In kg/20

    // geometric properties of the pedal
    // in mm
    public byte lengthPedal_AC;
    public byte horPos_AB;
    public byte verPos_AB;
    public byte lengthPedal_CB;

    // cubic spline params
    public float cubic_spline_param_a_0;
    public float cubic_spline_param_a_1;
    public float cubic_spline_param_a_2;
    public float cubic_spline_param_a_3;
    public float cubic_spline_param_a_4;

    public float cubic_spline_param_b_0;
    public float cubic_spline_param_b_1;
    public float cubic_spline_param_b_2;
    public float cubic_spline_param_b_3;
    public float cubic_spline_param_b_4;

    // PID settings
    public float PID_p_gain;
    public float PID_i_gain;
    public float PID_d_gain;

    public byte maxGameOutput;


}

public struct DAP_config_st
{
    public payloadHeader payloadHeader_;
    public payloadPedalConfig payloadPedalConfig_;
}



namespace User.PluginSdkDemo
{
    [PluginDescription("My plugin description")]
    [PluginAuthor("OpenSource")]
    [PluginName("DIY active pedal plugin")]
    public class DataPluginDemo : IPlugin, IDataPlugin, IWPFSettingsV2
    {


        public bool sendAbsSignal = false;
		public DAP_config_st dap_config_initial_st;





        //https://www.c-sharpcorner.com/uploadfile/eclipsed4utoo/communicating-with-serial-port-in-C-Sharp/
        public SerialPort[] _serialPort = new SerialPort[3] {new SerialPort("NA", 921600, Parity.None, 8, StopBits.One),
            new SerialPort("NA", 921600, Parity.None, 8, StopBits.One),
            new SerialPort("NA", 921600, Parity.None, 8, StopBits.One)};
        
    

        public bool serialPortConnected = false;


        public DataPluginDemoSettings Settings;

        /// <summary>
        /// Instance of the current plugin manager
        /// </summary>
        public PluginManager PluginManager { get; set; }

        /// <summary>
        /// Gets the left menu icon. Icon must be 24x24 and compatible with black and white display.
        /// </summary>
        public ImageSource PictureIcon => this.ToIcon(Properties.Resources.sdkmenuicon);

        /// <summary>
        /// Gets a short plugin title to show in left menu. Return null if you want to use the title as defined in PluginName attribute.
        /// </summary>
        public string LeftMenuTitle => "DIY active pedal plugin";

        /// <summary>
        /// Called one time per game data update, contains all normalized game data,
        /// raw data are intentionnally "hidden" under a generic object type (A plugin SHOULD NOT USE IT)
        ///
        /// This method is on the critical path, it must execute as fast as possible and avoid throwing any error
        ///
        /// </summary>
        /// <param name="pluginManager"></param>
        /// <param name="data">Current game data, including current and previous data frame.</param>
        public void DataUpdate(PluginManager pluginManager, ref GameData data)
        {
			
			bool sendAbsSignal_local_b = false;
            bool sendTcSignal_local_b = false;


            // Send ABS signal when triggered by the game
            if (data.GameRunning)
            {
                if (data.OldData != null && data.NewData != null)
                {
                    if (data.NewData.ABSActive > 0)
                    {
						sendAbsSignal_local_b = true;
                    }

                    if (data.NewData.TCActive > 0)
                    {
                        sendTcSignal_local_b = true;
                    }

                }
            }
			
			// Send ABS test signal if requested
            if (sendAbsSignal)
            {
				sendAbsSignal_local_b = true;
                sendTcSignal_local_b = true;
            }
			
			
			// Send ABS trigger signal via serial
			if (sendAbsSignal_local_b)
			{
				if (_serialPort[1].IsOpen)
                {
                    _serialPort[1].Write("2");
                }
			}

            // Send TC trigger signal via serial
            if (sendTcSignal_local_b)
            {
                if (_serialPort[2].IsOpen)
                {
                    _serialPort[2].Write("2");
                }
            }


        }
		
		

        /// <summary>
        /// Called at plugin manager stop, close/dispose anything needed here !
        /// Plugins are rebuilt at game change
        /// </summary>
        /// <param name="pluginManager"></param>
        public void End(PluginManager pluginManager)
        {
            //for(uint pedalIndex = 0; pedalIndex<3; pedalIndex++)
            //{
            //    Settings.selectedComPortNames[pedalIndex] = _serialPort[pedalIndex].PortName;


            //    SimHub.Logging.Current.Info("Diy active pedas plugin - Test 2: " + _serialPort[pedalIndex].PortName);


            //}
            
            // Save settings
            this.SaveCommonSettings("GeneralSettings", Settings);
        }

        /// <summary>
        /// Returns the settings control, return null if no settings control is required
        /// </summary>
        /// <param name="pluginManager"></param>
        /// <returns></returns>
        public System.Windows.Controls.Control GetWPFSettingsControl(PluginManager pluginManager)
        {
            return new SettingsControlDemo(this);
        }

        /// <summary>
        /// Called once after plugins startup
        /// Plugins are rebuilt at game change
        /// </summary>
        /// <param name="pluginManager"></param>
        public void Init(PluginManager pluginManager)
        {
            SimHub.Logging.Current.Info("Starting DIY active pedal plugin");

            // Load settings
            Settings = this.ReadCommonSettings<DataPluginDemoSettings>("GeneralSettings", () => new DataPluginDemoSettings());

            // Declare a property available in the property list, this gets evaluated "on demand" (when shown or used in formulas)
            this.AttachDelegate("CurrentDateTime", () => DateTime.Now);

            // Declare an event
            this.AddEvent("SpeedWarning");

            // Declare an action which can be called
            this.AddAction("IncrementSpeedWarning",(a, b) =>
            {
                Settings.SpeedWarningLevel++;
                SimHub.Logging.Current.Info("Speed warning changed");
            });

            // Declare an action which can be called
            this.AddAction("DecrementSpeedWarning", (a, b) =>
            {
                Settings.SpeedWarningLevel--;
            });


            //Settings.selectedJsonIndexLast[0]
            //SimHub.Logging.Current.Info("Diy active pedas plugin - Test 1");
            //SimHub.Logging.Current.Info("Diy active pedas plugin - COM port: " + Settings.selectedComPortNames[0]);







            // prepare serial port interfaces
   //         for (uint pedalIdx = 0; pedalIdx<3; pedalIdx++)
			//{
			//	if (_serialPort[pedalIdx].IsOpen)
			//	{
			//		_serialPort[pedalIdx].Close();
			//	}
				
			//	//_serialPort.Handshake = Handshake.None;
			//	_serialPort[pedalIdx].ReadTimeout = 2000;
			//	_serialPort[pedalIdx].WriteTimeout = 500;

   //             try
   //             {
   //                 _serialPort[pedalIdx].PortName = Settings.selectedComPortNames[pedalIdx];
   //             }
   //             catch (Exception caughtEx)
   //             {
   //             }


            //}

            //// check if Json config files are present, otherwise create new ones
            //for (uint jsonIndex = 0; jsonIndex < ComboBox_JsonFileSelected.Items.Count; jsonIndex++)
            //{
            //	// which config file is seleced
            //	string currentDirectory = Directory.GetCurrentDirectory();
            //	string dirName = currentDirectory + "\\PluginsData\\Common";
            //	string jsonFileName = ComboBox_JsonFileSelected(ComboBox_JsonFileSelected.Items[jsonIndex]).Text;
            //	string fileName = dirName + "\\" + jsonFileName + ".json";


            //	// Check if file already exists, otherwise create    
            //	if (!File.Exists(fileName))
            //	{
            //		// create default config
            //		// https://stackoverflow.com/questions/3275863/does-net-4-have-a-built-in-json-serializer-deserializer
            //		// https://learn.microsoft.com/en-us/dotnet/framework/wcf/feature-details/how-to-serialize-and-deserialize-json-data?redirectedfrom=MSDN
            //		var stream1 = new MemoryStream();
            //		var ser = new DataContractJsonSerializer(typeof(DAP_config_st));
            //		ser.WriteObject(stream1, dap_config_initial_st);

            //		stream1.Position = 0;
            //		StreamReader sr = new StreamReader(stream1);
            //		string jsonString = sr.ReadToEnd();

            //		System.IO.File.WriteAllText(fileName, jsonString);
            //	}
            //}



            dap_config_initial_st.payloadHeader_.payloadType = 100;
            dap_config_initial_st.payloadHeader_.version = 102;
            dap_config_initial_st.payloadHeader_.storeToEeprom = false;
            dap_config_initial_st.payloadPedalConfig_.pedalStartPosition = 35;
            dap_config_initial_st.payloadPedalConfig_.pedalEndPosition = 80;
            dap_config_initial_st.payloadPedalConfig_.maxForce = 90;
            dap_config_initial_st.payloadPedalConfig_.relativeForce_p000 = 0;
            dap_config_initial_st.payloadPedalConfig_.relativeForce_p020 = 20;
            dap_config_initial_st.payloadPedalConfig_.relativeForce_p040 = 40;
            dap_config_initial_st.payloadPedalConfig_.relativeForce_p060 = 60;
            dap_config_initial_st.payloadPedalConfig_.relativeForce_p080 = 80;
            dap_config_initial_st.payloadPedalConfig_.relativeForce_p100 = 100;
            dap_config_initial_st.payloadPedalConfig_.dampingPress = 0;
            dap_config_initial_st.payloadPedalConfig_.dampingPull = 0;
            dap_config_initial_st.payloadPedalConfig_.absFrequency = 5;
            dap_config_initial_st.payloadPedalConfig_.absAmplitude = 100;
            dap_config_initial_st.payloadPedalConfig_.lengthPedal_AC = 150;
            dap_config_initial_st.payloadPedalConfig_.horPos_AB = 215;
            dap_config_initial_st.payloadPedalConfig_.verPos_AB = 80;
            dap_config_initial_st.payloadPedalConfig_.lengthPedal_CB = 200;

            dap_config_initial_st.payloadPedalConfig_.maxGameOutput = 100;

            dap_config_initial_st.payloadPedalConfig_.cubic_spline_param_a_0 = 0;
            dap_config_initial_st.payloadPedalConfig_.cubic_spline_param_a_1 = 0;
            dap_config_initial_st.payloadPedalConfig_.cubic_spline_param_a_2 = 0;
            dap_config_initial_st.payloadPedalConfig_.cubic_spline_param_a_3 = 0;
            dap_config_initial_st.payloadPedalConfig_.cubic_spline_param_a_4 = 0;

            dap_config_initial_st.payloadPedalConfig_.cubic_spline_param_b_0 = 0;
            dap_config_initial_st.payloadPedalConfig_.cubic_spline_param_b_1 = 0;
            dap_config_initial_st.payloadPedalConfig_.cubic_spline_param_b_2 = 0;
            dap_config_initial_st.payloadPedalConfig_.cubic_spline_param_b_3 = 0;
            dap_config_initial_st.payloadPedalConfig_.cubic_spline_param_b_4 = 0;

            dap_config_initial_st.payloadPedalConfig_.PID_p_gain = 0.3f;
            dap_config_initial_st.payloadPedalConfig_.PID_i_gain = 50.0f;
            dap_config_initial_st.payloadPedalConfig_.PID_d_gain = 0.0f;


        }
    }
}