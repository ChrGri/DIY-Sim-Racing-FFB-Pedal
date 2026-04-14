using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace User.PluginSdkDemo
{
    public class PedalStatus : INotifyPropertyChanged
    {
        public int[] PedalForceInPercent { get; set; }
        public double[] PedalMaxForce { get; set; }
        public double[] PedalMinForce { get; set; }
        public string[] PedalConnectionStatus { get; set; }

        public PedalStatus() 
        {
            PedalForceInPercent = new int[3] { 0, 0, 0 };
            PedalMaxForce = new double[3] { 0, 0, 0 };
            PedalMinForce = new double[3] { 0, 0, 0 };
            PedalConnectionStatus = new string[3] { "Disconnected", "Disconnected", "Disconnected" };
        }

        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}
