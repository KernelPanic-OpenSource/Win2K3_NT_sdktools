if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_ClearPoolCorruption]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_ClearPoolCorruption]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_GetBucketComments]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_GetBucketComments]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_GetBucketCrashes]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_GetBucketCrashes]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_GetBucketData]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_GetBucketData]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_GetBucketStatsByAlias]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_GetBucketStatsByAlias]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_GetBucketStatus]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_GetBucketStatus]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_GetBucketsByAlias]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_GetBucketsByAlias]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_GetCommentActions]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_GetCommentActions]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_GetDeliveryTypes]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_GetDeliveryTypes]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_GetFollowUpIDs]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_GetFollowUpIDs]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_GetSolutionTypes]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_GetSolutionTypes]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_SetBucketBugNumber]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_SetBucketBugNumber]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_SetComment]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_SetComment]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_SetPoolCorruption]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_SetPoolCorruption]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_UpdateStaticDataBugID]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_UpdateStaticDataBugID]
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO




CREATE PROCEDURE DBGPortal_ClearPoolCorruption (
	@BucketID varchar(100)
)  AS

UPDATE  BucketToInt SET PoolCorruption = NULL WHERE BucketID = @BucketID



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE DBGPortal_GetBucketComments(
	@BucketID varchar(100)
)  AS


select EntryDate, CommentBy, Action, Comment from comments where BucketID = @BucketID order by entrydate desc
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE DBGPortal_GetBucketCrashes(
	@BucketID varchar(100)
)  AS



select top 300 bFullDump, SKU, BuildNo, Source, EntryDate, FilePath, Email from dbgportal_crashdatav3 where BucketID = @BucketID order by bFullDump desc, Source desc, Email desc, entrydate desc
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE DBGPortal_GetBucketData(
	@BucketID varchar(100)
)  AS


--select iBucket, FollowUp, BTI.iDriverName, DriverName, PoolCorruption, Platform, MoreData from BucketToInt as BTI
--left join FollowUpids as F on f.iFollowUp = BTI.iFollowUp
--left join DrNames as D on D.iDriverName = BTI.iDriverName
--where BTI.BucketID = @BucketID

select BTI.iBucket, FollowUp, BTI.iDriverName, DriverName, PoolCorruption, Platform, MoreData, CrashCount, SolutionID, R.BugID, [Area] from BucketToInt as BTI
left join DrNames as D on D.iDriverName = BTI.iDriverName
left join RaidBugs as R on BTI.BucketID = R.BucketID
inner join dbgportal_BucketDatav3 as BD on BD.BucketID = BTI.BucketID
where BTI.BucketID = @BucketID
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE DBGPortal_GetBucketStatsByAlias ( 
	@Alias varchar(50) 
) as

declare @TotalCrashes int
declare @AliasTotal int
declare @SolvedCrashes int
declare @RaidedCrashes int


select @TotalCrashes = sum(crashCount) from dbgportal_bucketdatav3 
select @AliasTotal = sum(crashCount) from dbgportal_bucketdatav3 where FollowUp = @alias
select @SolvedCrashes = sum(crashCount) from dbgportal_bucketdatav3 where FollowUp = @Alias and SolutionId is not null 
select @RaidedCrashes = sum(crashCount) from dbgportal_bucketdatav3 where FollowUp = @Alias and bugID is not null and SolutionID is null

if ( @SolvedCrashes is Null )
	set @SolvedCrashes = 0
if( @RaidedCrashes is null )
	set @RaidedCrashes = 0

select @TotalCrashes as Total, @AliasTotal as AliasTotal, @SolvedCrashes  as Solved, @RaidedCrashes as Raided, (@AliasTotal - @RaidedCrashes - @SolvedCrashes) as Untouched
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO

CREATE PROCEDURE DBGPortal_GetBucketStatus (
	@BucketID varchar(100)
)  AS




--select TOP 1 MAX(ActionID) as BiggestComment, [Action], CommentBy from Comments WHERE BucketID = @BucketID and ActionID != 7 and ActionID != 11 
--group by [Action], CommentBy ORDER BY BiggestComment DESC

if exists ( select * from comments where bucketID= @BucketID and ActionID = 8 )
	select top 1 ActionID as BiggestComment, [Action],  CommentBy from comments where BucketID = @BucketID and ActionID= 8 order by entrydate desc
else
	select top 1 ActionID as BiggestComment, [Action],  CommentBy from comments where BucketID = @BucketID order by entrydate desc
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE DBGPortal_GetBucketsByAlias (
	@Alias varchar(50)
)  AS


select top 25 BucketID, CrashCount, BugID from dbgportal_bucketDatav3 where FollowUp = @Alias and SolutionID is null order by CrashCount desc
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

CREATE PROCEDURE DBGPortal_GetCommentActions AS

SELECT * FROM CommentActions WHERE ActionID <= 5








GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE DBGPortal_GetDeliveryTypes  AS

SELECT DeliveryType FROM Solutions3.DBO.DeliveryTypes
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE DBGPortal_GetFollowUpIDs AS

select FollowUp, iFollowUP from FollowUpids
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO






CREATE PROCEDURE DBGPortal_GetSolutionTypes  AS

SELECT SolutionTypeName from Solutions3.DBO.SolutionTypes




GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

CREATE PROCEDURE DBGPortal_SetBucketBugNumber (
		@BucketID varchar(100),
		@BugID  int,
		@iBucket  int,
		@Area varchar(30)
	)  AS




	IF EXISTS( SELECT iBucket FROM RaidBugs WHERE BucketID = @BucketID  )
		UPDATE RaidBugs SET BugID=@BugID, Area=@Area  WHERE BucketID = @BucketID
	ELSE
		INSERT INTO RaidBugs (iBucket, BugID, BucketID, Area ) VALUES ( @iBucket, @BugID, @BucketID, @Area )






GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE DBGPortal_SetComment(
	@By varchar(20),
--	@Action nvarchar(50),
	@Action int,
	@Comment nvarchar(1000), 
	@BucketID varchar(100),
	@iBucket int
)  AS


DECLARE @ActionString nvarchar(50)

IF ( @Action = 7 )
	UPDATE Comments SET ActionID = 6, Action=(SELECT [Action] FROM CommentActions WHERE ActionID = 6)  WHERE BucketID=@BucketID and ActionID = 8

if ( @Action = 9 )
	UPDATE Comments SET ActionID = 10, Action=(SELECT [Action] FROM CommentActions WHERE ActionID = 10) WHERE BucketID=@BucketID and ActionID = 8



SELECT @ActionString = [Action] from CommentActions where ActionID = @Action


INSERT INTO Comments ( EntryDate, CommentBy, [Action], Comment, BucketID, ActionID, iBucket ) VALUES ( GETDATE(), @By, @ActionString, @Comment, @BucketID, @Action, @iBucket )
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO




CREATE PROCEDURE DBGPortal_SetPoolCorruption(
	@BucketID varchar(100)
)  AS

UPDATE BucketToInt SET PoolCorruption = 1 WHERE BucketID = @BucketID



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO





CREATE PROCEDURE DBGPortal_UpdateStaticDataBugID (
	@BucketID varchar(100),
	@BugID int
)  AS

UPDATE DBGPortal_BucketDataV3 SET BugID = @BugID WHERE BucketID = @BucketID




GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

