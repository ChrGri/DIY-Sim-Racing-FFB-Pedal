﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using User.PluginSdkDemo.UIElement;

namespace User.PluginSdkDemo.UIFunction
{
    /// <summary>
    /// KFTab.xaml 的互動邏輯
    /// </summary>
    public partial class GeneralSetting_KF : UserControl
    {
        public GeneralSetting_KF()
        {
            InitializeComponent();
            DataContext = this;
        }

        public static readonly DependencyProperty KF_selectionProperty =
    DependencyProperty.Register(nameof(KF_selection), typeof(int), typeof(GeneralSetting_KF),
        new FrameworkPropertyMetadata(0, FrameworkPropertyMetadataOptions.BindsTwoWayByDefault, OnPropertyChanged));

        public int KF_selection
        {
            get => (int)GetValue(KF_selectionProperty);
            set => SetValue(KF_selectionProperty, value);
        }
        public static readonly DependencyProperty KF_valueProperty =
    DependencyProperty.Register(nameof(KF_value), typeof(double), typeof(GeneralSetting_KF),
        new FrameworkPropertyMetadata(1.0, FrameworkPropertyMetadataOptions.BindsTwoWayByDefault, OnPropertyChanged));

        public double KF_value
        {
            get => (double)GetValue(KF_valueProperty);
            set => SetValue(KF_valueProperty, value);
        }

        private static void OnPropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            if (d is GeneralSetting_KF control)
            {
                //control.UpdateLabelContent();
            }
        }
        
        public event RoutedPropertyChangedEventHandler<double> KF_ValueChanged;
        public void KFValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            KF_value = e.NewValue;
            KF_ValueChanged?.Invoke(this, e);
        }
        public event RoutedEventHandler KF_SelectionChanged;
        public void KF_filter_order_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            KF_selection = KF_filter_order.SelectedIndex;
            KF_SelectionChanged?.Invoke(this, e);

        }

    }
}
