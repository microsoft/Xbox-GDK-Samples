/* Example of displaying CPU events in the capture. */

select s1.Value as 'Pix event', t.ProcThreadId, s2.Value as 'Thread name', cpu.*
from Strings s1, Strings s2, PixCpuExecution cpu, PixEventInfo pei, Threads t
where
    cpu.EventId = pei.Id
    and cpu.BeginTimestamp >= (select Value from CaptureFacts where Id = 2)
    and cpu.EndTimestamp <= (select Value from CaptureFacts where Id = 24)
    and pei.NameId = s1.Id
    and cpu.ThreadRowId = t.Id
    and t.ThreadNameId = s2.Id
limit 100