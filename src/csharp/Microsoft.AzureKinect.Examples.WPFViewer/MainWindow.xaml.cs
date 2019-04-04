using System;
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
using System.Windows.Media.Composition;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using Microsoft.AzureKinect;
using Microsoft.AzureKinect.WPF;

namespace K4aWpfTestApplication
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
        }

        private async void Window_Loaded(object sender, RoutedEventArgs e)
        {
            using (Device device = Device.Open(0))
            {
                device.StartCameras(new DeviceConfiguration {
                    color_format = ImageFormat.ColorBGRA32,
                    color_resolution = ColorResolution.r1080p,
                    depth_mode = DepthMode.NFOV_2x2Binned,
                    synchronized_images_only = true
                });

                while (true)
                {
                    using (Capture capture = await Task.Run(() => { return device.GetCapture(); }))
                    {
                        
                        colorImageViewPane.Source = capture.Color.CreateBitmapSource();
                        depthImageViewPane.Source = capture.Depth.CreateBitmapSource();
                    }
                }
            }
        }
    }
}
