   M S B A M L   `   `   `  ����I  EDiyActivePedal, Version=1.0.0.0, Culture=neutral, PublicKeyToken=null7 clr-namespace:User.PluginSdkDemoUser.PluginSdkDemo  5      U QMahApps.Metro, Version=1.5.0.23, Culture=neutral, PublicKeyToken=f4fb5a3c4d1e5b4fV;clr-namespace:MahApps.Metro.Controls;assembly=MahApps.MetroMahApps.Metro.Controls 5      2    ,User.PluginSdkDemo.UIElement.SliderWithLabel   5      R NAvalonDock, Version=4.72.1.0, Culture=neutral, PublicKeyToken=3e4669d2f30244f4N JWoteverLocalization, Version=1.0.0.0, Culture=neutral, PublicKeyToken=nullR NWindowsBase, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35T PWoteverCommon, Version=1.1.0.0, Culture=neutral, PublicKeyToken=35893a067a282a97\ XPresentationFramework, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35W SPresentationCore, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35` \AvalonDock.Themes.VS2013, Version=4.72.1.0, Culture=neutral, PublicKeyToken=3e4669d2f30244f4L 9http://schemas.microsoft.com/winfx/2006/xaml/presentation        5      R	 NSystem.Xaml, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089:x,http://schemas.microsoft.com/winfx/2006/xaml  	   5      Jmc;http://schemas.openxmlformats.org/markup-compatibility/2006  	   5      @d2http://schemas.microsoft.com/expression/blend/2008  	   5      2local clr-namespace:User.PluginSdkDemo  	   5      Pcontrols;clr-namespace:MahApps.Metro.Controls;assembly=MahApps.Metro  	   5        � 	Resources  5
      3  -System.Windows.Media.Effects.DropShadowEffect   mousedragging   Color   
BlurRadius   Opacity   ShadowDepth  mouseoutdragging  HorizontalSliderThumb �� 
TargetType �� Property  AccentColorBrush �� Value �� 
TargetType	 J� SnapsToDevicePixels
 �� RadiusX �� RadiusY ��Name  DisabledWhiteBrush J� IsHitTestVisible J� Opacity �� Triggers P� Property P� Value   HorizontalTrackLargeDecrease �Name �� EndPoint �� 
StartPoint �� Opacity � Color � Offset  HorizontalTrackValue  AccentColor  HorizontalSliderTemplate � RowDefinitions m�Name m� Fill m� 	Placement ]�Name ]� DecreaseRepeatButton �� DecreaseLarge ]� IncreaseRepeatButton  �� IncreaseLarge! ]� Thumb'  !MahApps.Metro.Controls.MetroThumb"  Name 	 HorizontalThumbWidth# �� 
TargetName�� 5   
   %^  &	        &	 G     &	 �     &	 n    &	 �    &	 p    7  7  7    5      $ White��65   $ 15Q�6C   $ 1Q�6S   $ 0Q�6_     5      $ White��68   $ 15Q�6F   $ 0Q�6V   $ 0Q�6b   �� 5      " n�62   .�5      ��  � ��# �  6/   �� 5       �@J�Opacity$	 0.9Q�6,   �� 5       � ��$	 0 0q�64   �� 5       � �� .  6.   �� 5       �@J�SnapsToDevicePixels . 68   �� 5       � ��$ 10��6*   �� 5       � �� 5      �� 5      " n�6*   .R�5      � #	 z	 6#   .+�5   "   �� #��z��6,   #��z��6P   $
 0��6{   $ 0��6�   �� 5   "   $ DisabledVisualElement��6,   #���  6K    .  $ 0Q�6�   5       5       P� 5!   "    �@n�
IsDragging . . �5"   &   �� # �   6?    �@J�Effect5#   #   P� 5$   "    �@n�
IsDragging .  . �5%   &   �� # �  6?    �@J�Effect5&   #   5(      5*      5+      �� 5,      " ��69   .�5-      ��  � |� .  6.   �� 5.       � �� .  6.   �� 5/       �@�OverridesDefaultStyle . 6:   �� 50       � �� 51      �� 52      " ��6*   .R�53      � $
 Root��6#   $��0 -1 0 0q�61   .+�54   "   �� $��1��6-   $��0 1 0 0q�68   $
 1��6I   $ 1��6U   ��55   &   �� 56   *   $ 0.5,1&�6>   $ 0.5,0&�6O   $	 0.9Q�6b   .��57   .   � # z��6;   $	 0.0Q�6`   � 58   .   $
 Gray��6;   $
 0.25Q�6H   � 59   .   $
 Gray��6;   $
 0.75Q�6H   � 5:   .   # z��6;   $	 1.0Q�6`   5;   +   5=   #   5>      5?      5A      5B      �� 5C      " ��61   .�5D      ��  � |� .  6.   �� 5E       � �� .  6.   �� 5F       �@�OverridesDefaultStyle . 6:   �� 5G       � �� 5H      �� 5I      " ��6*   .R�5J      � $
 Root��6#   $��0 -1 0 0q�61   .+�5K   "   �� $��4��6,   $��0 1 0 0q�67   $
 1��6H   $ 1��6T   ��5L   &   �� 5M   *   $ 0.5,1&�6>   $ 0.5,0&�6O   $	 0.9Q�6b   .��5N   .   � # z��6;   $	 0.0Q�6`   � 5O   .   # �  6;   $
 0.25Q�6a   � 5P   .   # �  6;   $
 0.75Q�6a   � 5Q   .   # z��6;   $	 1.0Q�6`   5R   +   5T   #   5U      5V      5X      5Y      �� 5Z      " ��6?   .R�5[      � $
 Root��6   .+�5\      � $ HorizontalTemplate��6   #��z��67    5]      �� 5^      $
��Auto �6,   �� 5_      $
��Auto �6,   #��z��6:   �� 5`      $
��Auto �6,   .+�5b      m� $ TopTick��6"   # z��63   $��4��6W   $	 Top=�6b   $��0��6r   $y�	Collapsed=�6   m� 5c      $ 
BottomTick��6"   # z��66   $��4��6Z   $ Bottom=�6e   $��2��6x   $y�	Collapsed=�6�   ]� 5d      $ 
PART_Track��6    $��1��64    5e      �� 5f   "   #��Z 6/   8��  6Y    5h      �� 5i   "   #��Z  6/   8�� 6Y   ! 5k        5l   "   $" HorizontalThumb��66   $��10��6O   ��. 8�� #��� 	 6�   5n      5o      5p       5q      P� 5r       �@��TickPlacement$ TopLeft=�67   . �5s      �� $# TopTick��67    � y�$ Visible=�6L   5t      P� 5u       �@��TickPlacement$ BottomRight=�67   . �5v      �� $# 
BottomTick��67    � y�$ Visible=�6O   5w      P� 5x       �@��TickPlacement$
 Both=�67   . �5y      �� $# 
BottomTick��67    � y�$ Visible=�6O   �� 5z      $# TopTick��67    � y�$ Visible=�6L   5{      5}      5~      .��5�      � .+�5�   
   �� .+�5�      �� $��Center=�5�      $
��Left=�6,   $��12��6G   $	��300��6S   $��0,0,0,8q�6_   $��Arial)�6p   $��10(�6�   $��0,0,0,0q�6�   ��5�      �� *LabelContent+$ �� RelativeSource$ �� % �� AncestorType$% UserControlL�& �� Mode$& OneWay=�5�   �   �� 5�      -   5        ' �� IsSnapToTickEnabled' . $��Center=�5�      $
��Left=�6,   $��10��6G    
 SliderStyle_single_H#��� 
 5�      ( � Minimum( 5�      �� *
MinValue+$ �� $% UserControlL�) � Maximum) 5�      �� *
MaxValue+$ �� $% UserControlL�* �� TickFrequency* 5�      �� *TickFrequency+$ �� $% UserControlL�+ � Value+ 5�      �� *SliderValue+$ �� $% UserControlL�$& TwoWay=���5�      �� *SliderLength+$ �� $% UserControlL�5�      5�      5�      5�      