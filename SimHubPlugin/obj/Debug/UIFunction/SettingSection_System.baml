   M S B A M L   `   `   `  ����I  EDiyActivePedal, Version=1.0.0.0, Culture=neutral, PublicKeyToken=nullM+clr-namespace:User.PluginSdkDemo.UIFunctionUser.PluginSdkDemo.UIFunction  5      K*clr-namespace:User.PluginSdkDemo.UIElementUser.PluginSdkDemo.UIElement  5      P LSimHub.Plugins, Version=1.0.9214.36797, Culture=neutral, PublicKeyToken=nullU;clr-namespace:SimHub.Plugins.Styles;assembly=SimHub.PluginsSimHub.Plugins.Styles 5      U QMahApps.Metro, Version=1.5.0.23, Culture=neutral, PublicKeyToken=f4fb5a3c4d1e5b4fV;clr-namespace:MahApps.Metro.Controls;assembly=MahApps.MetroMahApps.Metro.Controls 5	      9    3User.PluginSdkDemo.UIFunction.SettingSection_System   5      R NAvalonDock, Version=4.72.1.0, Culture=neutral, PublicKeyToken=3e4669d2f30244f4N JWoteverLocalization, Version=1.0.0.0, Culture=neutral, PublicKeyToken=nullR NWindowsBase, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35T PWoteverCommon, Version=1.1.0.0, Culture=neutral, PublicKeyToken=35893a067a282a97\ XPresentationFramework, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35W SPresentationCore, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35`	 \AvalonDock.Themes.VS2013, Version=4.72.1.0, Culture=neutral, PublicKeyToken=3e4669d2f30244f4L 9http://schemas.microsoft.com/winfx/2006/xaml/presentation       	 5      R
 NSystem.Xaml, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089:x,http://schemas.microsoft.com/winfx/2006/xaml  
   5      Jmc;http://schemas.openxmlformats.org/markup-compatibility/2006  
   5      @d2http://schemas.microsoft.com/expression/blend/2008  
   5      =local+clr-namespace:User.PluginSdkDemo.UIFunction  
   5      >localUI*clr-namespace:User.PluginSdkDemo.UIElement  
   5      Nstyles;clr-namespace:SimHub.Plugins.Styles;assembly=SimHub.Plugins  
   5      Pcontrols;clr-namespace:MahApps.Metro.Controls;assembly=MahApps.Metro  
   5	        � 	Resources  5         	TestBrush �� Color3  -System.Windows.Media.Effects.DropShadowEffect  mousedragging   Color   
BlurRadius   Opacity   ShadowDepth  mouseoutdragging  HorizontalSliderThumb �� 
TargetType �� Property  AccentColorBrush �� Value	 �� 
TargetType
 J� SnapsToDevicePixels �� RadiusX �� RadiusY ��Name  DisabledWhiteBrush J� IsHitTestVisible J� Opacity �� Triggers P� Property P� Value   HorizontalTrackLargeDecrease �Name �� EndPoint �� 
StartPoint �� Opacity � Color � Offset  HorizontalTrackValue  AccentColor 	 HorizontalSliderTemplate � RowDefinitions m�Name m� Fill m� 	Placement ]�Name ]� DecreaseRepeatButton �� DecreaseLarge  ]� IncreaseRepeatButton! �� IncreaseLarge" ]� Thumb'  !MahApps.Metro.Controls.MetroThumb#  Name 
 HorizontalThumbWidth$ �� 
TargetName  VerticalSliderThumb% �� BasedOn  VerticalTrackLargeDecrease  VerticalTrackValue  VerticalSliderTemplate& � ColumnDefinitions  VerticalThumbHeight  VerticalThumbWidth  SliderStyle_single_H  ControlBorderBrush  	TextBrush' �� Triggers  RangeSliderHorizontal(  "MahApps.Metro.Controls.RangeSlider( ��Name) �� Orientation* � Maximum+ m� Maximum, � Minimum- m� Minimum.   TickFrequency/ m� TickFrequency0 ��Name1 n�Name2 � Cursor3   MinRangeWidth  RangeSliderVertical  RangeSliderStyle�� 5   
   %J&  &	        &	      &	 f     &	 �     &	 �    &	     &		 �    7  7  7  &	 5    7  &	 �    &	 �    &	 ]    7  7  7  &	 �    &	 �    &	 '    &	 �$    7  7  �� 5      $ White��60     5      $ White��65   $ 15Q�6C   $ 1Q�6S   $ 0Q�6_     5      $ White��68   $ 15Q�6F   $ 0Q�6V   $ 0Q�6b   �� 5      " n�62   .�5      ��  � ��# �  6/   �� 5       �@J�Opacity$	 0.9Q�6,   �� 5       � ��$	 0 0q�64   �� 5       � �� .  6.   �� 5       �@J�SnapsToDevicePixels . 68   �� 5       � ��$ 10��6*   �� 5       � �� 5      �� 5      "	 n�6*   .R�5      � #
 z
 6#   .+�5   "   �� #��z��6,   #��z��6P   $ 0��6{   $ 0��6�   �� 5   "   $ DisabledVisualElement��6,   #���  6K    .  $ 0Q�6�   5!       5"      P� 5#   "    �@n�
IsDragging . . �5$   &   �� # �  6?    �@J�Effect5%   #   P� 5&   "    �@n�
IsDragging .  . �5'   &   �� # �  6?    �@J�Effect5(   #   5*      5,      5-      �� 5.      " ��69   .�5/      ��  � |� .  6.   �� 50       � �� .  6.   �� 51       �@�OverridesDefaultStyle . 6:   �� 52       � �� 53      �� 54      "	 ��6*   .R�55      � $
 Root��6#   $��0 -1 0 0q�61   .+�56   "   �� $��1��6-   $��0 1 0 0q�68   $ 1��6I   $ 1��6U   ��57   &   �� 58   *   $ 0.5,1&�6>   $ 0.5,0&�6O   $	 0.9Q�6b   .��59   .   � # z��6;   $	 0.0Q�6`   � 5:   .   $
 Gray��6;   $
 0.25Q�6H   � 5;   .   $
 Gray��6;   $
 0.75Q�6H   � 5<   .   # z��6;   $	 1.0Q�6`   5=   +   5?   #   5@      5A      5C      5D      �� 5E      " ��61   .�5F      ��  � |� .  6.   �� 5G       � �� .  6.   �� 5H       �@�OverridesDefaultStyle . 6:   �� 5I       � �� 5J      �� 5K      "	 ��6*   .R�5L      � $
 Root��6#   $��0 -1 0 0q�61   .+�5M   "   �� $��4��6,   $��0 1 0 0q�67   $ 1��6H   $ 1��6T   ��5N   &   �� 5O   *   $ 0.5,1&�6>   $ 0.5,0&�6O   $	 0.9Q�6b   .��5P   .   � # z��6;   $	 0.0Q�6`   � 5Q   .   # �  6;   $
 0.25Q�6a   � 5R   .   # �  6;   $
 0.75Q�6a   � 5S   .   # z��6;   $	 1.0Q�6`   5T   +   5V   #   5W      5X      5Z      5[      �� 5\      "	 ��6?   .R�5]      � $
 Root��6   .+�5^      � $ HorizontalTemplate��6   #��z��67    5_      �� 5`      $
��Auto �6,   �� 5a      $
��Auto �6,   #��z��6:   �� 5b      $
��Auto �6,   .+�5d      m� $ TopTick��6"   # z��63   $��4��6W   $	 Top=�6b   $��0��6r   $y�	Collapsed=�6   m� 5e      $ 
BottomTick��6"   # z��66   $��4��6Z   $ Bottom=�6e   $��2��6x   $y�	Collapsed=�6�   ]� 5f      $ 
PART_Track��6    $��1��64    5g      �� 5h   "   #��Z 6/   8��  6Y     5j      �� 5k   "   #��Z! 6/   8�� 6Y   " 5m        5n   "   $# HorizontalThumb��66   $��10��6O   ��. 8�� #��� 
 6�   5p      5q      5r       5s      P� 5t       �@��TickPlacement$ TopLeft=�67   . �5u      �� $$ TopTick��67    � y�$ Visible=�6L   5v      P� 5w       �@��TickPlacement$ BottomRight=�67   . �5x      �� $$ 
BottomTick��67    � y�$ Visible=�6O   5y      P� 5z       �@��TickPlacement$
 Both=�67   . �5{      �� $$ 
BottomTick��67    � y�$ Visible=�6O   �� 5|      $$ TopTick��67    � y�$ Visible=�6L   5}      5      �� 5�      8%   " n�6a   .�5�      ��  � ��$	 0 0q�64   5�      �� 5�      " ��67   .�5�      ��  � |� .  6.   �� 5�       � �� .  6.   �� 5�       �@�OverridesDefaultStyle . 6:   �� 5�       � �� 5�      �� 5�      "	 ��6*   .R�5�      � $
 Root��6#   $��-1 0 0 0q�61   .+�5�   "   �� 
�������$��1 0 0 0q�68   $��1��6I   $	 0.8Q�6S   5�      5�      5�      5�      �� 5�      " ��6/   .�5�      ��  � |� .  6.   �� 5�       � �� .  6.   �� 5�       �@�OverridesDefaultStyle . 6:   �� 5�       � �� 5�      �� 5�      "	 ��6*   .R�5�      � $
 Root��6#   $��-1 0 0 0q�61   .+�5�   "   �� $��0 0 0 0q�6,   $��10��6=   ��5�   &   �� 5�   *   $ 1,0.5&�6>   $ 0,0.5&�6O   $	 0.9Q�6b   .��5�   .   � # z��6;   $	 0.0Q�6`   � 5�   .   # �  6;   $
 0.25Q�6a   � 5�   .   # �  6;   $
 0.75Q�6a   � 5�   .   # z��6;   $	 1.0Q�6`   5�   +   5�   #   5�      5�      5�      5�      �� 5�      "	 ��6=   .R�5�      � $
 Root��6   .+�5�      � $ HorizontalTemplate��6   #��z��67   & 5�      �� 5�      $
��Auto �6/   �� 5�      #��z��6/   $
��Auto �6U   �� 5�      $
��Auto �6/   .+�5�      m� $ TopTick��6"   $��0��63   # z��6C   $
 Left=�6g   $y�	Collapsed=�6x   $��4��6�   m� 5�      $ 
BottomTick��6"   $��2��66   # z��6F   $ Right=�6j   $y�	Collapsed=�6|   $��4��6�   ]� 5�      $ 
PART_Track��6    $��1��64    5�      �� 5�   "   #��Z 6/   8��  6Y     5�      �� 5�   "   #��Z! 6/   8�� 6Y   " 5�        5�   "   $# VerticalThumb��66   #���  6M   ��. 8�� #���  6�   5�      5�      5�       5�      P� 5�       �@��TickPlacement$ TopLeft=�67   . �5�      �� $$ TopTick��67    � y�$ Visible=�6L   5�      P� 5�       �@��TickPlacement$ BottomRight=�67   . �5�      �� $$ 
BottomTick��67    � y�$ Visible=�6O   5�      P� 5�       �@��TickPlacement$
 Both=�67   . �5�      �� $$ 
BottomTick��67    � y�$ Visible=�6O   �� 5�      $$ TopTick��67    � y�$ Visible=�6L   5�      5�      �� 5�      " ��61   .�5�      ��  � ��# �  60   �� 5�       � ��$ 1q�64   �� 5�       � ��# �  6/   �� 5�       � �� .  6.   �� 5�       �@�Maximum$	 100Q�6,   �� 5�       �@�Minimum$ 0Q�6,   �� 5�       � ��# � 	 6-   �� 5�       �@�Value$ 0Q�6*   ' 5�      P� 5�       �@��Orientation$ Vertical=�65   . �5�      ��  � ��# �  65   5�      5�      �� 5�      "	  6<   .R�5�      �� $( PART_Container��6   $��Stretch=�65   $) Vertical=�6S   $��Center=�6j   .+�5�      m� $ PART_TopTick��6   # z��64   $��4��6X   $��5 0 5 2q�6c   #+ z* 6t   #- z, 6�   $	 Top=�6�   #/ z. 6�   $y�	Collapsed=�6�   �� 5�      $( PART_RangeSliderContainer��6!   
������ $��Stretch=�6]   $) 
Horizontal=�6{   $��Center=�6�   .+�5�      �� $0 PART_LeftEdge��6'   #���  6>   n� 5�      $1 PART_LeftThumb��6    $2 Arrow��68   $��10��6G   $��10��6S   #���  6^   n� 5�      $1 PART_MiddleThumb��6    $
2 Hand��6:   #��z3 6H   ��5�      �� 5�   "   " n�6(   .�5�   &   ��  � �� 5�   *   �� 5�   .   .R�5�   2   �� #���  6<   $��4��6f   $��0 1 0 0q�6q   $	 0.8Q�6�   5�   /   5�   '   5�   #   5�      n� 5�      $1 PART_RightThumb��6    $2 Arrow��69   $��10��6H   #���  6T   $��10��6�   �� 5�      $0 PART_RightEdge��6'   #���  6?   5�      m� 5�      $ PART_BottomTick��6   # z��67   $��4��6[   $��5 2 5 0q�6f   #+ z* 6w   #- z, 6�   $	 Top=�6�   #/ z. 6�   $y�	Collapsed=�6�   5�       5�      P� 5�       �@ TickPlacement$ TopLeft=�67   . �5�      �� $$ PART_TopTick��67    � y�$ Visible=�6Q   5�      P� 5�       �@ TickPlacement$ BottomRight=�67   . �5�      �� $$ PART_BottomTick��67    � y�$ Visible=�6T   5�      P� 5�       �@ TickPlacement$
 Both=�67   . �5�      �� $$ PART_BottomTick��67    � y�$ Visible=�6T   �� 5�      $$ PART_TopTick��67    � y�$ Visible=�6Q   5�      5�      �� 5      "	  6:   .R�5     �� $( PART_Container��6   $��Center=�65   $) 
Horizontal=�6R   $��Stretch=�6k   .+�5     m� $ PART_TopTick��6   # z��64   $��0 5 2 5q�6X   #+ z* 6i   #- z, 6�   $
 Left=�6�   #/ z. 6�   $y�	Collapsed=�6�   $��4��6	  �� 5     $( PART_RangeSliderContainer��6!   
������ $��Center=�6]   $) Vertical=�6z   $��Stretch=�6�   .+�5     �� $0 PART_RightEdge��6'   #���  6?   $��Stretch=�6t   n� 5     $1 PART_RightThumb��6    $2 Arrow��69   $��10��6H   #���  6T   $��10��6�   n� 5     $1 PART_MiddleThumb��6    $
2 Hand��6:   #��z3 6H   ��5     �� 5  "   " n�6(   .�5	  &   ��  � �� 5
  *   �� 5  .   .R�5  2   �� #���  6<   $��0 0 0 0q�6f   $��3��6w   $	 0.7Q�6�   5  /   5  '   5  #   5     n� 5     $1 PART_LeftThumb��6    $2 Arrow��68   $��10��6G   #���  6S   $��10��6�   �� 5     $0 PART_LeftEdge��6'   #���  6>   $��Stretch=�6s   5     m� 5     $ PART_BottomTick��6   # z��67   $��2 5 0 5q�6[   #+ z* 6l   #- z, 6�   $ Right=�6�   #/ z. 6�   $y�	Collapsed=�6�   $��4��6  5      5     P� 5      �@ TickPlacement$ TopLeft=�67   . �5     �� $$ PART_TopTick��67    � y�$ Visible=�6Q   5     P� 5      �@ TickPlacement$ BottomRight=�67   . �5     �� $$ PART_BottomTick��67    � y�$ Visible=�6T   5     P� 5      �@ TickPlacement$
 Both=�67   . �5      �� $$ PART_BottomTick��67    � y�$ Visible=�6T   �� 5!     $$ PART_TopTick��67    � y�$ Visible=�6Q   5"     5$     �� 5%     "  6-   .�5&     ��  � ��
 �����6/   �� 5'      � ��8   6-   ' 5(     P� 5)      �@ Orientation$ Vertical=�65   . �5*     ��  � ��8  65   5+     5-     5.     .��50     � .+�51  
   �� ���	#FF4E4E4E$	��274��6(   4 �� CornerRadius$4 5,5,5,5��65   $��0,0,0,0q�6L   $	��160��6f   $
��Left=�6r   $��0,3,0,0q�6�   ._�52     �� $��Center=�6   $	��274��66   $	��Top=�6C   $	��160��6[   .+�53     �� $��System Setting��6   $��20��61   $	��150��6=   $��0,0,0,0q�6I   $��Center=�6[   $��Arial Black)�6}   $
��Left=�6�   $��10(�6�   $��2,0,0,0q�6�   �� 55     $��20��6   $	��160��6)   $) 
Horizontal=�65   $
��Left=�6N   $��Center=�6i   $��5,2,0,0q�6�   .+�56     *  $SimHub.Plugins.Styles.SHToggleButton  -   5        5  Name$'5 !CheckBox_Pedal_ESPNow_autoconnect��56  ,   $�� ��6W   6 d� 	IsChecked$6 FalseY�6�   $��10(�6�   $��35��6�   $��15��6�   $
��Left=�6  $��Arial)�6   7 � ToolTip$7  ��63  8 J� RenderTransformOrigin$8 0.5,0.5&�6>  $��	-10,0,0,0q�6^  �� 57     $��Auto Connect to Bridge��6   $��20��6=   $	��130��6I   $��0,0,0,0q�6U   $��Center=�6g   $��Arial)�6�   $
��Left=�6�   $��10(�6�   $��4,0,0,0q�6�   58     �� 59     $��20��6   $	��160��6)   $) 
Horizontal=�65   $
��Left=�6N   $��Center=�6i   $��5,2,0,0q�6�   .+�5:       -   5        $#5 CheckBox_using_CDC_for_bridge��5:  ,   $�� ��6S   $6 FalseY�6�   $��10(�6�   $��35��6�   $��15��6�   $
��Left=�6�   $��Arial)�6  $e7 _This function is for performance testing, leave it off if you dont know what the function doing��6'  $8 0.5,0.5&�6�  $��	-10,0,0,0q�6�  {�.  6�  �� 5;     $��CDC Serial for Bridge��6   $��20��6<   $	��130��6H   $��0,0,0,0q�6T   $��Center=�6f   $��Arial)�6�   $
��Left=�6�   $��10(�6�   $��4,0,0,0q�6�   5<     �� 5=     $��20��6   $	��160��6)   $) 
Horizontal=�65   $
��Left=�6N   $��Center=�6i   $��5,2,0,0q�6�   .+�5>       -   5        $5 Debug_check��5>  ,   $�� ��6A   $6 FalseY�6�   $��10(�6�   $��35��6�   $��15��6�   $
��Left=�6�   $��Arial)�6�   $"7 Some opttions were hidden...��6�   $8 0.5,0.5&�6  $��	-10,0,0,0q�68  �� 5?     $��Advanced Mode��6   $��20��64   $	��130��6@   $��0,0,0,0q�6L   $��Center=�6^   $��Arial)�6�   $
��Left=�6�   $��10(�6�   $��4,0,0,0q�6�   5@     5A     5B     5C     5D     