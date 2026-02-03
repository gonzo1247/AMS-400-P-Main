--
-- RBAC AMS Permissions

-- role
DELETE FROM `rbac_permissions` WHERE `id` BETWEEN 180 AND 184;
INSERT INTO `rbac_permissions` (`id`, `name`) VALUE
(180, 'Observer - AMS'),
(181, 'Operator - AMS'),
(182, 'Coordinator - AMS'),
(183, 'Manager - AMS'),
(184, 'Admmin - AMS');

-- group permissions
DELETE FROM `rbac_group_permissions` WHERE `permissionId` BETWEEN 180 AND 184;
INSERT INTO `rbac_group_permissions` (`groupId`, `permissionId`) VALUES
(1, 180),
(2, 181),
(3, 182),
(4, 183),
(5, 184);

-- permissions
SET @PERMID := 400;
DELETE FROM `rbac_permissions` WHERE `id` BETWEEN @PERMID AND @PERMID+18;
INSERT INTO `rbac_permissions` (`id`, `name`) VALUES
(@PERMID, 'Can open Caller Manager'),
(@PERMID+1, 'Can Open Emplyee Manager'),
(@PERMID+2, 'Can Create Ticket'),
(@PERMID+3, 'Can Show Tickets'),
(@PERMID+4, 'Can Ticket modifie'),
(@PERMID+5, 'Can Open TV'),
(@PERMID+6, 'Can Open Machine and Lines'),
(@PERMID+7, 'Can Search in Tickets open'),
-- Ticket Details Page
(@PERMID+8, 'Can Change Title or Description'),
(@PERMID+9, 'Can Change Priority'),
(@PERMID+10, 'Can Change Status'),
(@PERMID+11, 'Can Assign or Unassign Employee'),
(@PERMID+12, 'Can Upload Files'),
(@PERMID+13, 'Can add Ticket Comment'),
(@PERMID+14, 'Can add Spare part'),
(@PERMID+15, 'Can remive Spare part'),
(@PERMID+16, 'Can create Report'),
(@PERMID+17, 'Can modifie Report'),
(@PERMID+18, 'Can close Ticket');



-- linked_permissions
DELETE FROM `rbac_linked_permissions` WHERE `linkedId` BETWEEN @PERMID AND @PERMID+18;
INSERT INTO `rbac_linked_permissions` (`id`, `linkedId`) VALUES
(182, @PERMID),
(182, @PERMID+1),
(181, @PERMID+2),
(180, @PERMID+3),
(182, @PERMID+4),
(182, @PERMID+5),
(182, @PERMID+6),
(182, @PERMID+7),
(182, @PERMID+8),
(182, @PERMID+9),
(181, @PERMID+10),
(182, @PERMID+11),
(182, @PERMID+12),
(181, @PERMID+13),
(181, @PERMID+14),
(181, @PERMID+15),
(181, @PERMID+16),
(181, @PERMID+17),
(182, @PERMID+18);