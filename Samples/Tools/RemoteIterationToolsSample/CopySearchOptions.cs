namespace RemoteIterationToolsSample
{
    public class CopySearchOptions
    {
        public string? IncludeFilePattern { get; set; }
        public string? ExcludeFilePattern { get; set; }
        public string? ExcludeDirPattern { get; set; }
        public ulong IncludeFileAttributes { get; set; }
        public ulong ExcludeFileAttributes { get; set; }
    }
}
