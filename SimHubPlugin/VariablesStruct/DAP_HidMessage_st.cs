using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;
namespace User.PluginSdkDemo
{
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public unsafe struct Dap_hidmessage_st
    {
        public byte payloadType;
        public byte magicKey1;
        public byte magicKey2;
        public byte length;
        public fixed byte text[235];
    }
}
