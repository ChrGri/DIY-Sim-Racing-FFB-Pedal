using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace User.PluginSdkDemo
{
    // https://stackoverflow.com/questions/14344305/best-way-to-structure-class-struct-in-c-sharp
    //[StructLayout(LayoutKind.Sequential, Pack = 1)]
    //[Serializable]
    public struct payloadHeader
    {
        // structure identification via payload
        public byte payloadType;

        // variable to check if structure at receiver matched version from transmitter
        public byte version;

        public byte storeToEeprom;
        public byte PedalTag;
    }
}
