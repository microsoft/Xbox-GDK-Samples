//-----------------------------------------------------------------------------
// SampleDBContext.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License file under the project root for
// license information.
//-----------------------------------------------------------------------------

using Microsoft.EntityFrameworkCore;
using Newtonsoft.Json;
using Xbox.AgentProtocol;
using XboxAgentServerSample.Models;

namespace XboxAgentServerSample.Data
{
    public class SampleDBContext : DbContext
    {
        public SampleDBContext(DbContextOptions<SampleDBContext> options)
            : base(options)
        { }

        //  These DbSets will contain data that we want to be shared across our
        //  servers, so we store them in a persistent SQL database in Azure.  The sample
        //  by default looks for the app settings ConnectionString "GameServicePersistentDB"
        //  if not found, then it will default to use an in-memory database for simplicity
        //  But all deployed code should be using a real DB for these tables to prevent
        //  data loss and unnecessary network traffic.
        public DbSet<XboxAgentDBEntry> LinkedDevkits { get; set; }


        //  These helper functions are to address the compiler warnings of possible null
        //  reference returns when doing v => JsonConvert.DeserializeObject<object>(v).
        //  This ensures that a non-null value is returned

        //Dictionary<string, string>
        private static Dictionary<string, string> DeserializeDictionaryWithNullCheck(string v)
        {
            var returnVal = new Dictionary<string, string>();

            if (v != null)
            {
                JsonConvert.DeserializeObject<Dictionary<string, string>>(v);
            }
            return returnVal;
        }

        //ICollection<string>
        private static ICollection<string> DeserializeStringCollectionWithNullCheck(string v)
        {
            ICollection<string> returnVal = new List<string>();

            if (v != null)
            {
                JsonConvert.DeserializeObject<ICollection<string>>(v);
            }
            return returnVal;
        }

        //CancelCurrentJob
        private static CancelCurrentJob DeserializeCancelCurrentJobWithNullCheck(string v)
        {
            var returnVal = new CancelCurrentJob();

            if (v != null)
            {
                JsonConvert.DeserializeObject<CancelCurrentJob>(v);
            }
            return returnVal;
        }

        //DownloadFilesJob
        private static DownloadFilesJob DeserializeDownloadFilesJobWithNullCheck(string v)
        {
            var returnVal = new DownloadFilesJob();

            if (v != null)
            {
                JsonConvert.DeserializeObject<ICollection<string>>(v);
            }
            return returnVal;
        }

        //DownloadFilesJobResult
        private static DownloadFilesJobResult DeserializeDownloadFilesJobResultWithNullCheck(string v)
        {
            var returnVal = new DownloadFilesJobResult();

            if (v != null)
            {
                JsonConvert.DeserializeObject<DownloadFilesJobResult>(v);
            }
            return returnVal;
        }

        //ExecuteCommandJob
        private static ExecuteCommandJob DeserializeExecuteCommandJobWithNullCheck(string v)
        {
            var returnVal = new ExecuteCommandJob();

            if (v != null)
            {
                JsonConvert.DeserializeObject<ExecuteCommandJob>(v);
            }
            return returnVal;
        }

        //ExecuteCommandJobResult
        private static ExecuteCommandJobResult DeserializeExecuteCommandJobResultWithNullCheck(string v)
        {
            var returnVal = new ExecuteCommandJobResult();

            if (v != null)
            {
                JsonConvert.DeserializeObject<ExecuteCommandJobResult>(v);
            }
            return returnVal;
        }

        //ICollection<FailedFileTransferResult>
        private static ICollection<FailedFileTransferResult> DeserializeFailedFileTransferResultCollectionWithNullCheck(string v)
        {
            ICollection<FailedFileTransferResult> returnVal = new List<FailedFileTransferResult>();

            if (v != null)
            {
                JsonConvert.DeserializeObject<ICollection<FailedFileTransferResult>>(v);
            }
            return returnVal;
        }

        //ICollection<FileUpload>
        private static ICollection<FileUpload> DeserializeFileUploadCollectionWithNullCheck(string v)
        {
            ICollection<FileUpload> returnVal = new List<FileUpload>();

            if (v != null)
            {
                JsonConvert.DeserializeObject<ICollection<FileUpload>>(v);
            }
            return returnVal;
        }

        //UploadFilesJob
        private static UploadFilesJob DeserializeUploadFilesJobWithNullCheck(string v)
        {
            var returnVal = new UploadFilesJob();

            if (v != null)
            {
                JsonConvert.DeserializeObject<UploadFilesJob>(v);
            }
            return returnVal;
        }

        //UploadFilesJobResult
        private static UploadFilesJobResult DeserializeUploadFilesJobResultWithNullCheck(string v)
        {
           var returnVal = new UploadFilesJobResult();

            if (v != null)
            {
                JsonConvert.DeserializeObject<UploadFilesJobResult>(v);
            }
            return returnVal;
        }



        protected override void OnModelCreating(ModelBuilder modelBuilder)
        {
            base.OnModelCreating(modelBuilder);

            modelBuilder.Entity<FileDownload>()
                .Property(b => b.HttpHeaders)
                .HasConversion(
                    v => JsonConvert.SerializeObject(v),
                    v => DeserializeDictionaryWithNullCheck(v));

            modelBuilder.Entity<FileUpload>()
                .Property(b => b.HttpHeaders)
                .HasConversion(
                    v => JsonConvert.SerializeObject(v),
                    v => DeserializeDictionaryWithNullCheck(v));

            modelBuilder.Entity<Package>()
                .Property(b => b.Aumids)
                .HasConversion(
                    v => JsonConvert.SerializeObject(v),
                    v => DeserializeStringCollectionWithNullCheck(v));

            modelBuilder.Entity<CancelCurrentJob>()
                .HasNoKey();

            modelBuilder.Entity<RequestedJob>()
               .Property(b => b.CancelCurrentJob)
               .HasConversion(
                   v => JsonConvert.SerializeObject(v),
                   v => DeserializeCancelCurrentJobWithNullCheck(v));

            modelBuilder.Entity<DownloadFilesJob>()
                .HasNoKey();

            modelBuilder.Entity<DownloadFilesJobResult>()
               .Property(b => b.Job)
               .HasConversion(
                   v => JsonConvert.SerializeObject(v),
                   v => DeserializeDownloadFilesJobWithNullCheck(v));

            modelBuilder.Entity<RequestedJob>()
               .Property(b => b.DownloadFilesJob)
               .HasConversion(
                   v => JsonConvert.SerializeObject(v),
                   v => DeserializeDownloadFilesJobWithNullCheck(v));

            modelBuilder.Entity<DownloadFilesJobResult>()
                .HasNoKey();

            modelBuilder.Entity<LastProcessedJobResult>()
               .Property(b => b.DownloadFilesJobResult)
               .HasConversion(
                   v => JsonConvert.SerializeObject(v),
                   v => DeserializeDownloadFilesJobResultWithNullCheck(v));

            modelBuilder.Entity<ExecuteCommandJob>()
                .HasNoKey();

            modelBuilder.Entity<ExecuteCommandJobResult>()
               .Property(b => b.Job)
               .HasConversion(
                   v => JsonConvert.SerializeObject(v),
                   v => DeserializeExecuteCommandJobWithNullCheck(v));

            modelBuilder.Entity<RequestedJob>()
               .Property(b => b.ExecuteCommandJob)
               .HasConversion(
                   v => JsonConvert.SerializeObject(v),
                   v => DeserializeExecuteCommandJobWithNullCheck(v));

            modelBuilder.Entity<ExecuteCommandJobResult>()
                .HasNoKey();

            modelBuilder.Entity<LastProcessedJobResult>()
               .Property(b => b.ExecuteCommandJobResult)
               .HasConversion(
                   v => JsonConvert.SerializeObject(v),
                   v => DeserializeExecuteCommandJobResultWithNullCheck(v));

            modelBuilder.Entity<FailedFileTransferResult>()
                .HasNoKey();

            modelBuilder.Entity<UploadFilesJobResult>()
               .Property(b => b.FailedFileUploadResults)
               .HasConversion(
                   v => JsonConvert.SerializeObject(v),
                   v => DeserializeFailedFileTransferResultCollectionWithNullCheck(v));

            modelBuilder.Entity<FileDownload>()
                .HasNoKey();

            modelBuilder.Entity<FileUpload>()
                .HasNoKey();

            modelBuilder.Entity<UploadFilesJob>()
                .Property(b => b.Files)
                .HasConversion(
                   v => JsonConvert.SerializeObject(v),
                   v => DeserializeFileUploadCollectionWithNullCheck(v));

            modelBuilder.Entity<Kit>()
                .HasKey(b => b.Hostname);

            modelBuilder.Entity<Executable>()
                .HasKey(b => b.FileName);

            modelBuilder.Entity<KitHeartbeatRequest>()
                .HasNoKey();

            modelBuilder.Entity<XboxAgentDevkitState>()
                .Property(b => b.LatestAgentState)
                .HasConversion(
                    v => JsonConvert.SerializeObject(v),
                    v => JsonConvert.DeserializeObject<KitHeartbeatRequest>(v));

            modelBuilder.Entity<Package>()
                .HasKey(b => b.FullName);

            modelBuilder.Entity<UploadFilesJob>()
                .HasNoKey();

            modelBuilder.Entity<RequestedJob>()
                .Property(b => b.UploadFilesJob)
                .HasConversion(
                    v => JsonConvert.SerializeObject(v),
                    v => DeserializeUploadFilesJobWithNullCheck(v));

            modelBuilder.Entity<UploadFilesJobResult>()
                .Property(b => b.Job)
                .HasConversion(
                    v => JsonConvert.SerializeObject(v),
                    v => DeserializeUploadFilesJobWithNullCheck(v));

            modelBuilder.Entity<UploadFilesJobResult>()
                .HasNoKey();

            modelBuilder.Entity<LastProcessedJobResult>()
                .Property(b => b.UploadFilesJobResult)
                .HasConversion(
                    v => JsonConvert.SerializeObject(v),
                    v => DeserializeUploadFilesJobResultWithNullCheck(v));
        }

        public DbSet<XboxAgentServerSample.Models.XboxAgentDevkitState> XboxAgentDevkitState { get; set; } = default!;
    }
}
