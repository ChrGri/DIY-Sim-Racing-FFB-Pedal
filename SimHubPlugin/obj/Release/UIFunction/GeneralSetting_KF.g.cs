﻿#pragma checksum "..\..\..\UIFunction\GeneralSetting_KF.xaml" "{8829d00f-11b8-4213-878b-770e8597ac16}" "BB8BC262B98816E55E0098B9BAA57BB057B93377BE509F5971D8BE4EEA712C0C"
//------------------------------------------------------------------------------
// <auto-generated>
//     Dieser Code wurde von einem Tool generiert.
//     Laufzeitversion:4.0.30319.42000
//
//     Änderungen an dieser Datei können falsches Verhalten verursachen und gehen verloren, wenn
//     der Code erneut generiert wird.
// </auto-generated>
//------------------------------------------------------------------------------

using AvalonDock;
using AvalonDock.Controls;
using AvalonDock.Converters;
using AvalonDock.Layout;
using AvalonDock.Themes;
using MahApps.Metro.Controls;
using SimHub.Plugins.Styles;
using SimHub.Plugins.UI;
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
    /// GeneralSetting_KF
    /// </summary>
    public partial class GeneralSetting_KF : System.Windows.Controls.UserControl, System.Windows.Markup.IComponentConnector {
        
        
        #line 315 "..\..\..\UIFunction\GeneralSetting_KF.xaml"
        [System.Diagnostics.CodeAnalysis.SuppressMessageAttribute("Microsoft.Performance", "CA1823:AvoidUnusedPrivateFields")]
        internal System.Windows.Controls.ComboBox KF_filter_order;
        
        #line default
        #line hidden
        
        
        #line 327 "..\..\..\UIFunction\GeneralSetting_KF.xaml"
        [System.Diagnostics.CodeAnalysis.SuppressMessageAttribute("Microsoft.Performance", "CA1823:AvoidUnusedPrivateFields")]
        internal User.PluginSdkDemo.UIElement.SliderWithLabel Slider_KF;
        
        #line default
        #line hidden
        
        
        #line 331 "..\..\..\UIFunction\GeneralSetting_KF.xaml"
        [System.Diagnostics.CodeAnalysis.SuppressMessageAttribute("Microsoft.Performance", "CA1823:AvoidUnusedPrivateFields")]
        internal SimHub.Plugins.Styles.SHToggleButton Checkbox_joystick_denoise;
        
        #line default
        #line hidden
        
        
        #line 334 "..\..\..\UIFunction\GeneralSetting_KF.xaml"
        [System.Diagnostics.CodeAnalysis.SuppressMessageAttribute("Microsoft.Performance", "CA1823:AvoidUnusedPrivateFields")]
        internal User.PluginSdkDemo.UIElement.SliderWithLabel Slider_KF_Joystick;
        
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
            System.Uri resourceLocater = new System.Uri("/DiyActivePedal;component/uifunction/generalsetting_kf.xaml", System.UriKind.Relative);
            
            #line 1 "..\..\..\UIFunction\GeneralSetting_KF.xaml"
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
            this.KF_filter_order = ((System.Windows.Controls.ComboBox)(target));
            
            #line 315 "..\..\..\UIFunction\GeneralSetting_KF.xaml"
            this.KF_filter_order.SelectionChanged += new System.Windows.Controls.SelectionChangedEventHandler(this.KF_filter_order_SelectionChanged);
            
            #line default
            #line hidden
            return;
            case 2:
            this.Slider_KF = ((User.PluginSdkDemo.UIElement.SliderWithLabel)(target));
            return;
            case 3:
            this.Checkbox_joystick_denoise = ((SimHub.Plugins.Styles.SHToggleButton)(target));
            
            #line 331 "..\..\..\UIFunction\GeneralSetting_KF.xaml"
            this.Checkbox_joystick_denoise.Checked += new System.Windows.RoutedEventHandler(this.Checkbox_joystick_denoise_Checked);
            
            #line default
            #line hidden
            
            #line 331 "..\..\..\UIFunction\GeneralSetting_KF.xaml"
            this.Checkbox_joystick_denoise.Unchecked += new System.Windows.RoutedEventHandler(this.Checkbox_joystick_denoise_Unchecked);
            
            #line default
            #line hidden
            return;
            case 4:
            this.Slider_KF_Joystick = ((User.PluginSdkDemo.UIElement.SliderWithLabel)(target));
            return;
            }
            this._contentLoaded = true;
        }
    }
}

