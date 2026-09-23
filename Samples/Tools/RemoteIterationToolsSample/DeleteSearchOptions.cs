namespace RemoteIterationToolsSample
{
    public class DeleteSearchOptions
    {
        public string? IncludeFilePattern { get; init; }
        public string? ExcludeFilePattern { get; init; }
        public string? ExcludeDirPattern { get; init; }
        public ulong IncludeFileAttributes { get; init; }
        public ulong ExcludeFileAttributes { get; init; }
        public ulong IncludeDirAttributes { get; init; }
        public ulong ExcludeDirAttributes { get; init; }
    }
}
