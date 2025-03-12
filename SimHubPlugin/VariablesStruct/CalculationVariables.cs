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

        public CalculationVariables()
        {
            PedalCurrentForce = 0;
            PedalCurrentTravel = 0;
        }
    }
}
