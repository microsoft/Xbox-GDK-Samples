/* Example of displaying GPU events in the capture. */

select s1.Value as 'Pix Event', s2.Value as 'Api queue', gpu.*
from Strings s1, Strings s2, PixGpuExecution gpu, PixEventInfo pei, ApiCommandQueue q
where
    gpu.EventId = pei.Id
    and gpu.BeginTimestamp > (select Value from CaptureFacts where Id = 2)
    and gpu.EndTimestamp < (select Value from CaptureFacts where Id = 24)
    and pei.NameId = s1.Id
    and gpu.ApiCommandQueueId = q.Id
    and q.NameId = s2.Id
limit 100