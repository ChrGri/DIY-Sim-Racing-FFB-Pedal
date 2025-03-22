using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Media;

namespace User.PluginSdkDemo
{
    public class CalculationVariables
    {
        public int PedalCurrentTravel { get; set; }
        public int PedalCurrentForce { get; set; }
        public bool SendAbsSignal { get; set; }
        public bool IsUIRefreshNeeded { get; set; }
        public bool Update_CV1_textbox { get; set; }
        public bool Update_CV2_textbox { get; set; }

        public double[] Force_curve_Y = new double[100];

        public int current_pedal_travel_state { get; set; }
        public byte pedal_state_in_ratio { get; set; }
        public bool isDragging { get; set; }

        public Point offset;

        public SolidColorBrush lightcolor;
        public SolidColorBrush defaultcolor;
        public SolidColorBrush Red_Warning;
        public SolidColorBrush White_Default;
        public string btn_SendConfig_Content;
        public string btn_SendConfig_tooltip;
        public bool[] dumpPedalToResponseFile_clearFile;
        public bool[] dumpPedalToResponseFile;
        public bool Update_Profile_Checkbox_b;
        public string current_profile = "NA";
        public uint profile_index;
        public bool ForceUpdate_b;
        public uint UpdateChannel;
        public CalculationVariables()
        {
            PedalCurrentForce = 0;
            PedalCurrentTravel = 0;
            SendAbsSignal = false;
            IsUIRefreshNeeded = false;
            Update_CV1_textbox = false;
            Update_CV2_textbox = false;
            pedal_state_in_ratio = 0;
            current_pedal_travel_state = 0;
            isDragging = false;
            lightcolor = new SolidColorBrush();
            defaultcolor = new SolidColorBrush();
            Red_Warning = new SolidColorBrush(Color.FromArgb(255, 244, 67, 67));
            White_Default = new SolidColorBrush(Color.FromArgb(255, 255, 255, 255));
            offset = new Point();
            btn_SendConfig_Content = "Send Config to Pedal";
            btn_SendConfig_tooltip = "Send Config to Pedal and save in storage";
            dumpPedalToResponseFile_clearFile = new bool[3] { false, false, false };
            dumpPedalToResponseFile = new bool[3] { false, false, false };
            Update_Profile_Checkbox_b = false;
            profile_index = 0;
            ForceUpdate_b = false;
            UpdateChannel = 0;
        }
    }
}
