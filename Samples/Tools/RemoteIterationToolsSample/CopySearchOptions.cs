namespace RemoteIterationToolsSample
{
    public class CopySearchOptions
    {
        public string? IncludeFilePattern { get; set; }
        public string? ExcludeFilePattern { get; set; }
        public string? ExcludeDirPattern { get; set; }
        public uint IncludeFileAttributes { get; set; }
        public uint ExcludeFileAttributes { get; set; }
        public uint IncludeDirectoryAttributes { get; set; }
        public uint ExcludeDirectoryAttributes { get; set; }
    }
}
