using System.IO;
using System.Windows;

namespace RemoteIterationToolsSample
{
    public partial class FileAttributeSelector : System.Windows.Controls.UserControl
    {
        public static readonly DependencyProperty LabelProperty =
            DependencyProperty.Register(nameof(Label), typeof(string), typeof(FileAttributeSelector), new PropertyMetadata(""));

        public string Label
        {
            get => (string)GetValue(LabelProperty);
            set => SetValue(LabelProperty, value);
        }

        public FileAttributeSelector()
        {
            InitializeComponent();
        }

        public uint Mask
        {
            get
            {
                // These four Windows flags are the subset supported by the RIT filters.
                FileAttributes attributes = 0;
                if (ReadOnlyCheckBox.IsChecked == true)
                    attributes |= FileAttributes.ReadOnly;
                if (HiddenCheckBox.IsChecked == true)
                    attributes |= FileAttributes.Hidden;
                if (SystemCheckBox.IsChecked == true)
                    attributes |= FileAttributes.System;
                if (ArchiveCheckBox.IsChecked == true)
                    attributes |= FileAttributes.Archive;
                return (uint)attributes;
            }
        }
    }
}
