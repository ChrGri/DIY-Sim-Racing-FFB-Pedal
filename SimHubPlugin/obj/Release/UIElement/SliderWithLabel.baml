   M S B A M L   `   `   `  ���� I  EDiyActivePedal, Version=1.0.0.0, Culture=neutral, PublicKeyToken=null7 clr-namespace:User.PluginSdkDemoUser.PluginSdkDemo  U QMahApps.Metro, Version=1.5.0.23, Culture=neutral, PublicKeyToken=f4fb5a3c4d1e5b4fV;clr-namespace:MahApps.Metro.Controls;assembly=MahApps.MetroMahApps.Metro.Controls 2    ,User.PluginSdkDemo.UIElement.SliderWithLabel   R NAvalonDock, Version=4.72.1.0, Culture=neutral, PublicKeyToken=3e4669d2f30244f4N JWoteverLocalization, Version=1.0.0.0, Culture=neutral, PublicKeyToken=nullR NWindowsBase, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35T PWoteverCommon, Version=1.1.0.0, Culture=neutral, PublicKeyToken=35893a067a282a97\ XPresentationFramework, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35W SPresentationCore, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35` \AvalonDock.Themes.VS2013, Version=4.72.1.0, Culture=neutral, PublicKeyToken=3e4669d2f30244f4L 9http://schemas.microsoft.com/winfx/2006/xaml/presentation        R	 NSystem.Xaml, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089:x,http://schemas.microsoft.com/winfx/2006/xaml  	   Jmc;http://schemas.openxmlformats.org/markup-compatibility/2006  	   @d2http://schemas.microsoft.com/expression/blend/2008  	   2local clr-namespace:User.PluginSdkDemo  	   Pcontrols;clr-namespace:MahApps.Metro.Controls;assembly=MahApps.Metro  	     � 	Resources  3  -System.Windows.Media.Effects.DropShadowEffect   mousedragging   Color   
BlurRadius   Opacity   ShadowDepth  mouseoutdragging  HorizontalSliderThumb �� 
TargetType �� Property  AccentColorBrush �� Value �� 
TargetType	 J� SnapsToDevicePixels
 �� RadiusX �� RadiusY ��Name  DisabledWhiteBrush J� IsHitTestVisible J� Opacity �� Triggers P� Property P� Value   HorizontalTrackLargeDecrease �Name �� EndPoint �� 
StartPoint �� Opacity � Color � Offset  HorizontalTrackValue  AccentColor  HorizontalSliderTemplate � RowDefinitions m�Name m� Fill m� 	Placement ]�Name ]� DecreaseRepeatButton �� DecreaseLarge ]� IncreaseRepeatButton  �� IncreaseLarge! ]� Thumb'  !MahApps.Metro.Controls.MetroThumb"  Name 	 HorizontalThumbWidth# �� 
TargetName�� %�  &	        &	 *     &	 T     &	     &	 \    &	 �    7  7  7    $ White��$ 15Q�$ 1Q�$ 0Q�  $ White��$ 15Q�$ 0Q�$ 0Q��� " n�.���  � ��# �  ��  �@J�Opacity$	 0.9Q���  � ��$	 0 0q���  � �� .  ��  �@J�SnapsToDevicePixels . ��  � ��$ 10����  � �� �� " n�.R�� #	 z	 .+��� #��z��#��z��$
 0��$ 0���� $ DisabledVisualElement��#���   .  $ 0Q� P�  �@n�
IsDragging . . ��� # �    �@J�EffectP�  �@n�
IsDragging .  . ��� # �   �@J�Effect�� " ��.���  � |� .  ��  � �� .  ��  �@�OverridesDefaultStyle . ��  � �� �� " ��.R�� $
 Root��$��0 -1 0 0q�.+��� $��1��$��0 1 0 0q�$
 1��$ 1������ $ 0.5,1&�$ 0.5,0&�$	 0.9Q�.��� # z��$	 0.0Q�� $
 Gray��$
 0.25Q�� $
 Gray��$
 0.75Q�� # z��$	 1.0Q��� " ��.���  � |� .  ��  � �� .  ��  �@�OverridesDefaultStyle . ��  � �� �� " ��.R�� $
 Root��$��0 -1 0 0q�.+��� $��4��$��0 1 0 0q�$
 1��$ 1������ $ 0.5,1&�$ 0.5,0&�$	 0.9Q�.��� # z��$	 0.0Q�� # �  $
 0.25Q�� # �  $
 0.75Q�� # z��$	 1.0Q��� " ��.R�� $
 Root��.+�� $ HorizontalTemplate��#��z�� �� $
��Auto ��� $
��Auto �#��z���� $
��Auto �.+�m� $ TopTick��# z��$��4��$	 Top=�$��0��$y�	Collapsed=�m� $ 
BottomTick��# z��$��4��$ Bottom=�$��2��$y�	Collapsed=�]� $ 
PART_Track��$��1�� �� #��Z 8��   �� #��Z  8�� !   $" HorizontalThumb��$��10����. 8�� #��� 	  P�  �@��TickPlacement$ TopLeft=�. ��� $# TopTick�� � y�$ Visible=�P�  �@��TickPlacement$ BottomRight=�. ��� $# 
BottomTick�� � y�$ Visible=�P�  �@��TickPlacement$
 Both=�. ��� $# 
BottomTick�� � y�$ Visible=��� $# TopTick�� � y�$ Visible=�.��� .+��� .+��� $��Center=�$
��Left=�$��12��$	��300��$��0,0,0,8q�$��Arial)�$��10(�$��0,0,0,0q����� *LabelContent+$ �� RelativeSource$ �� % �� AncestorType$% UserControlL�& �� Mode$& OneWay=��� -   ' �� IsSnapToTickEnabled' . $��Center=�$
��Left=�$��10�� 
 SliderStyle_single_H#��� 
 ( � Minimum( �� *
MinValue+$ �� $% UserControlL�) � Maximum) �� *
MaxValue+$ �� $% UserControlL�* �� TickFrequency* �� *TickFrequency+$ �� $% UserControlL�+ � Value+ �� *SliderValue+$ �� $% UserControlL�$& TwoWay=����� *SliderLength+$ �� $% UserControlL�