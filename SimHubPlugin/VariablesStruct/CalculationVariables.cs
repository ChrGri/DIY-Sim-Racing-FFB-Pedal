using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

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
        public CalculationVariables()
        {
            PedalCurrentForce = 0;
            PedalCurrentTravel = 0;
            SendAbsSignal = false;
            IsUIRefreshNeeded = false;
            Update_CV1_textbox = false;
            Update_CV2_textbox = false;
        }
    }
}
