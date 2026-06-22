/* Example of displaying the list of all PIX counters. */

with recursive
counter_group(Id, ParentId, name) as
(
        select c.Id, c.GroupId, s.Value
        from PixCounterInfo c, Strings s
        where c.NameId = s.Id
    union all
        select cg.Id, pcg.ParentGroupId, s.Value
        from PixCounterGroup pcg, counter_group cg, Strings s
        where pcg.Id = cg.ParentId and pcg.NameId = s.Id
)
select cg.Id, group_concat(cg.Name, ' -> ') as 'Counter tree'
from counter_group cg group by cg.Id