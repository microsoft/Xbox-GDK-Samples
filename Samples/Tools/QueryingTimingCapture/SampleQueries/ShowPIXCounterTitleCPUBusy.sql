/* Example of displaying the PIX counter: Title CPU -> % Busy. */

select c.Value, c.timestamp, sc.Value as 'Counter name'
from PixCounters c, Strings sc, Strings sl1, Strings sl2, PixCounterGroup pcg2, PixCounterGroup pcg1, PixCounterInfo ci
where
    sl1.Value = '% Busy'
    and sl2.Value = 'Title CPU'
    and ci.NameId = sc.Id
    and pcg1.NameId = sl1.Id
    and pcg2.NameId = sl2.Id
    and c.CounterId = ci.Id
    and ci.GroupId = pcg1.Id
    and pcg1.ParentGroupId = pcg2.Id
    and c.Timestamp >= (select Value from CaptureFacts where Id = 2)
    and c.Timestamp <= (select Value from CaptureFacts where Id = 24)
order by c.Timestamp asc
limit 100