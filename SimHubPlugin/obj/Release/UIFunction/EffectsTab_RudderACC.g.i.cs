﻿#pragma checksum "..\..\..\UIFunction\EffectsTab_RudderACC.xaml" "{8829d00f-11b8-4213-878b-770e8597ac16}" "5C35D1C174AE0AA7E7F40ABD4058AC3B471931A848A74AFA9BE29905CA4246EC"
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
    /// EffectsTab_RudderACC
    /// </summary>
    public partial class EffectsTab_RudderACC : System.Windows.Controls.UserControl, System.Windows.Markup.IComponentConnector {
        
        
        #line 308 "..\..\..\UIFunction\EffectsTab_RudderACC.xaml"
        [System.Diagnostics.CodeAnalysis.SuppressMessageAttribute("Microsoft.Performance", "CA1823:AvoidUnusedPrivateFields")]
        internal SimHub.Plugins.Styles.SHToggleButton checkbox_Rudder_ACC_effect;
        
        #line default
        #line hidden
        
        
        #line 312 "..\..\..\UIFunction\EffectsTab_RudderACC.xaml"
        [System.Diagnostics.CodeAnalysis.SuppressMessageAttribute("Microsoft.Performance", "CA1823:AvoidUnusedPrivateFields")]
        internal SimHub.Plugins.Styles.SHToggleButton Checkbox_Rudder_ACC_WindForce;
        
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
            System.Uri resourceLocater = new System.Uri("/DiyActivePedal;component/uifunction/effectstab_rudderacc.xaml", System.UriKind.Relative);
            
            #line 1 "..\..\..\UIFunction\EffectsTab_RudderACC.xaml"
            System.Windows.Application.LoadComponent(this, resourceLocater);
            
            #line default
            #line hidden
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
            this.checkbox_Rudder_ACC_effect = ((SimHub.Plugins.Styles.SHToggleButton)(target));
            
            #line 308 "..\..\..\UIFunction\EffectsTab_RudderACC.xaml"
            this.checkbox_Rudder_ACC_effect.Checked += new System.Windows.RoutedEventHandler(this.checkbox_Rudder_ACC_effect_Checked);
            
            #line default
            #line hidden
            
            #line 308 "..\..\..\UIFunction\EffectsTab_RudderACC.xaml"
            this.checkbox_Rudder_ACC_effect.Unchecked += new System.Windows.RoutedEventHandler(this.checkbox_Rudder_ACC_effect_Unchecked);
            
            #line default
            #line hidden
            return;
            case 2:
            this.Checkbox_Rudder_ACC_WindForce = ((SimHub.Plugins.Styles.SHToggleButton)(target));
            
            #line 312 "..\..\..\UIFunction\EffectsTab_RudderACC.xaml"
            this.Checkbox_Rudder_ACC_WindForce.Checked += new System.Windows.RoutedEventHandler(this.Checkbox_Rudder_ACC_WindForce_Checked);
            
            #line default
            #line hidden
            
            #line 312 "..\..\..\UIFunction\EffectsTab_RudderACC.xaml"
            this.Checkbox_Rudder_ACC_WindForce.Unchecked += new System.Windows.RoutedEventHandler(this.Checkbox_Rudder_ACC_WindForce_Unchecked);
            
            #line default
            #line hidden
            return;
            }
            this._contentLoaded = true;
        }
    }
}

