﻿#pragma checksum "..\..\..\UIFunction\EffectsTab_RPMRudder.xaml" "{8829d00f-11b8-4213-878b-770e8597ac16}" "D76055AF99917423EF1EF0E411D22B5B2B3ECA3A71EF84FB3E53E862E4E1C579"
//------------------------------------------------------------------------------
// <auto-generated>
//     這段程式碼是由工具產生的。
//     執行階段版本:4.0.30319.42000
//
//     對這個檔案所做的變更可能會造成錯誤的行為，而且如果重新產生程式碼，
//     變更將會遺失。
// </auto-generated>
//------------------------------------------------------------------------------

using AvalonDock;
using AvalonDock.Controls;
using AvalonDock.Converters;
using AvalonDock.Layout;
using AvalonDock.Themes;
using MahApps.Metro.Controls;
using SimHub.Plugins.Styles;
using System;
using System.Diagnostics;
using System.Windows;
using System.Windows.Automation;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Ink;
using System.Windows.Input;
using System.Windows.Markup;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Media.Effects;
using System.Windows.Media.Imaging;
using System.Windows.Media.Media3D;
using System.Windows.Media.TextFormatting;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Windows.Shell;
using User.PluginSdkDemo.UIElement;
using User.PluginSdkDemo.UIFunction;
using WoteverCommon.WPF;
using WoteverCommon.WPF.AutoGrid;
using WoteverCommon.WPF.Behaviors;
using WoteverCommon.WPF.Converters;
using WoteverCommon.WPF.Styles;
using WoteverLocalization;


namespace User.PluginSdkDemo.UIFunction {
    
    
    /// <summary>
    /// EffectsTab_RPMRudder
    /// </summary>
    public partial class EffectsTab_RPMRudder : System.Windows.Controls.UserControl, System.Windows.Markup.IComponentConnector {
        
        
        #line 308 "..\..\..\UIFunction\EffectsTab_RPMRudder.xaml"
        [System.Diagnostics.CodeAnalysis.SuppressMessageAttribute("Microsoft.Performance", "CA1823:AvoidUnusedPrivateFields")]
        internal SimHub.Plugins.Styles.SHToggleButton checkbox_enable_RPM_rudder;
        
        #line default
        #line hidden
        
        
        #line 311 "..\..\..\UIFunction\EffectsTab_RPMRudder.xaml"
        [System.Diagnostics.CodeAnalysis.SuppressMessageAttribute("Microsoft.Performance", "CA1823:AvoidUnusedPrivateFields")]
        internal User.PluginSdkDemo.UIElement.SliderWithLabel Slider_RPM_AMP_rudder;
        
        #line default
        #line hidden
        
        
        #line 314 "..\..\..\UIFunction\EffectsTab_RPMRudder.xaml"
        [System.Diagnostics.CodeAnalysis.SuppressMessageAttribute("Microsoft.Performance", "CA1823:AvoidUnusedPrivateFields")]
        internal System.Windows.Controls.Label label_RPM_freq_rudder;
        
        #line default
        #line hidden
        
        
        #line 317 "..\..\..\UIFunction\EffectsTab_RPMRudder.xaml"
        [System.Diagnostics.CodeAnalysis.SuppressMessageAttribute("Microsoft.Performance", "CA1823:AvoidUnusedPrivateFields")]
        internal System.Windows.Controls.Label label_RPM_freq_min_rudder;
        
        #line default
        #line hidden
        
        
        #line 318 "..\..\..\UIFunction\EffectsTab_RPMRudder.xaml"
        [System.Diagnostics.CodeAnalysis.SuppressMessageAttribute("Microsoft.Performance", "CA1823:AvoidUnusedPrivateFields")]
        internal System.Windows.Controls.Label label_RPM_freq_max_rudder;
        
        #line default
        #line hidden
        
        
        #line 320 "..\..\..\UIFunction\EffectsTab_RPMRudder.xaml"
        [System.Diagnostics.CodeAnalysis.SuppressMessageAttribute("Microsoft.Performance", "CA1823:AvoidUnusedPrivateFields")]
        internal MahApps.Metro.Controls.RangeSlider Rangeslider_RPM_freq_rudder;
        
        #line default
        #line hidden
        
        private bool _contentLoaded;
        
        /// <summary>
        /// InitializeComponent
        /// </summary>
        [System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [System.CodeDom.Compiler.GeneratedCodeAttribute("PresentationBuildTasks", "4.0.0.0")]
        public void InitializeComponent() {
            if (_contentLoaded) {
                return;
            }
            _contentLoaded = true;
            System.Uri resourceLocater = new System.Uri("/DiyActivePedal;component/uifunction/effectstab_rpmrudder.xaml", System.UriKind.Relative);
            
            #line 1 "..\..\..\UIFunction\EffectsTab_RPMRudder.xaml"
            System.Windows.Application.LoadComponent(this, resourceLocater);
            
            #line default
            #line hidden
        }
        
        [System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [System.CodeDom.Compiler.GeneratedCodeAttribute("PresentationBuildTasks", "4.0.0.0")]
        [System.Diagnostics.CodeAnalysis.SuppressMessageAttribute("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode")]
        internal System.Delegate _CreateDelegate(System.Type delegateType, string handler) {
            return System.Delegate.CreateDelegate(delegateType, this, handler);
        }
        
        [System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [System.CodeDom.Compiler.GeneratedCodeAttribute("PresentationBuildTasks", "4.0.0.0")]
        [System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never)]
        [System.Diagnostics.CodeAnalysis.SuppressMessageAttribute("Microsoft.Design", "CA1033:InterfaceMethodsShouldBeCallableByChildTypes")]
        [System.Diagnostics.CodeAnalysis.SuppressMessageAttribute("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity")]
        [System.Diagnostics.CodeAnalysis.SuppressMessageAttribute("Microsoft.Performance", "CA1800:DoNotCastUnnecessarily")]
        void System.Windows.Markup.IComponentConnector.Connect(int connectionId, object target) {
            switch (connectionId)
            {
            case 1:
            this.checkbox_enable_RPM_rudder = ((SimHub.Plugins.Styles.SHToggleButton)(target));
            
            #line 308 "..\..\..\UIFunction\EffectsTab_RPMRudder.xaml"
            this.checkbox_enable_RPM_rudder.Checked += new System.Windows.RoutedEventHandler(this.checkbox_enable_RPM_rudder_Checked);
            
            #line default
            #line hidden
            
            #line 308 "..\..\..\UIFunction\EffectsTab_RPMRudder.xaml"
            this.checkbox_enable_RPM_rudder.Unchecked += new System.Windows.RoutedEventHandler(this.checkbox_enable_RPM_rudder_Unchecked);
            
            #line default
            #line hidden
            return;
            case 2:
            this.Slider_RPM_AMP_rudder = ((User.PluginSdkDemo.UIElement.SliderWithLabel)(target));
            return;
            case 3:
            this.label_RPM_freq_rudder = ((System.Windows.Controls.Label)(target));
            return;
            case 4:
            this.label_RPM_freq_min_rudder = ((System.Windows.Controls.Label)(target));
            return;
            case 5:
            this.label_RPM_freq_max_rudder = ((System.Windows.Controls.Label)(target));
            return;
            case 6:
            this.Rangeslider_RPM_freq_rudder = ((MahApps.Metro.Controls.RangeSlider)(target));
            
            #line 320 "..\..\..\UIFunction\EffectsTab_RPMRudder.xaml"
            this.Rangeslider_RPM_freq_rudder.LowerValueChanged += new MahApps.Metro.Controls.RangeParameterChangedEventHandler(this.Rangeslider_RPM_freq_rudder_LowerValueChanged);
            
            #line default
            #line hidden
            
            #line 320 "..\..\..\UIFunction\EffectsTab_RPMRudder.xaml"
            this.Rangeslider_RPM_freq_rudder.UpperValueChanged += new MahApps.Metro.Controls.RangeParameterChangedEventHandler(this.Rangeslider_RPM_freq_rudder_UpperValueChanged);
            
            #line default
            #line hidden
            return;
            }
            this._contentLoaded = true;
        }
    }
}

