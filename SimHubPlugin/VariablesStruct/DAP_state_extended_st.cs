﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace User.PluginSdkDemo
{
    //[StructLayout(LayoutKind.Sequential, Pack = 1)]
    //[Serializable]
    public struct DAP_state_extended_st
    {
        public payloadHeader payloadHeader_;
        public payloadPedalState_Extended payloadPedalExtendedState_;
        public payloadFooter payloadFooter_;
    }
}
