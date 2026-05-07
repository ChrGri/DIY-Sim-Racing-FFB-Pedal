using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;

namespace DiyFfbPedal
{
    public class PedalStatus : INotifyPropertyChanged
    {
        public double[] PedalForceInPercent { get; set; }
        public double[] PedalMaxForce { get; set; }
        public double[] PedalMinForce { get; set; }
        public double[] PedalForceInKg { get; set; }
        public string[] PedalConnectionStatus { get; set; }

        public PedalStatus()
        {
            PedalForceInPercent = new double[3] { 0.0d, 0.0d, 0.0d };
            PedalMaxForce = new double[3] { 0, 0, 0 };
            PedalMinForce = new double[3] { 0, 0, 0 };
            PedalForceInKg = new double[3] { 0, 0, 0 };
            PedalConnectionStatus = new string[3] { "Disconnected", "Disconnected", "Disconnected" };
        }

        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
        public void UpdatePedalStatus()
        {
            for (int i = 0; i < 3; i++)
            {
                PedalForceInKg[i] = PedalForceInPercent[i] * (PedalMaxForce[i] - PedalMinForce[i]) / 100.0d + PedalMinForce[i];
            }
        }
        public void UpdatePluginProperties(dynamic pluginManager, PedalStatus instance, DIY_FFB_Pedal Name)
        {
            PropertyInfo[] properties = instance.GetType().GetProperties();

            foreach (PropertyInfo prop in properties)
            {
                if (prop.PropertyType.IsArray)
                {
                    Array arrayValue = (Array)prop.GetValue(this);

                    if (arrayValue != null && arrayValue.Length==3)
                    {
                        for (int i = 0; i < 3; i++)
                        {
                            string tmp = $"PedalStatus.{prop.Name}.{PedalConstStrings.PedalID[i]}";

                            object itemValue = arrayValue.GetValue(i);
                            pluginManager.SetPropertyValue(tmp, Name.GetType(), itemValue);
                        }
                    }
                }
            }
        }
        public void AddPluginProperties(dynamic pluginManager, PedalStatus instance, DIY_FFB_Pedal Name)
        {
            PropertyInfo[] properties = instance.GetType().GetProperties();

            foreach (PropertyInfo prop in properties)
            {
                if (prop.PropertyType.IsArray)
                {
                    Array arrayValue = (Array)prop.GetValue(this);

                    if (arrayValue != null && arrayValue.Length == 3)
                    {
                        for (int i = 0; i < 3; i++)
                        {
                            string tmp = $"PedalStatus.{prop.Name}.{PedalConstStrings.PedalID[i]}";

                            object itemValue = arrayValue.GetValue(i);
                            pluginManager.AddProperty(tmp, Name.GetType(), itemValue);
                        }
                    }
                }
            }
        }
    }
    
    
}

   
