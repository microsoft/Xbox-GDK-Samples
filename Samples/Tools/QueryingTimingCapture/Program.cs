namespace QueryingTimingCapture;

public class Program
{
    [System.STAThreadAttribute()]
    public static int Main()
    {
        QueryingTimingCapture.App app = new QueryingTimingCapture.App();
        app.InitializeComponent();
        app.Run();

        return 0;
    }
}
