--
-- "$Id: mxml.sql 150 2004-05-21 03:59:17Z mike $"
--
-- Database schema for the EPM web pages.
--
-- This SQL file is specifically for use with MySQL.
--
-- Revision History:
--
--   M. Sweet   05/17/2004   Initial revision.
--   M. Sweet   05/19/2004   Added link, poll, and vote tables.
--   M. Sweet   05/20/2004   Changes for MySQL
--   M. Sweet   08/05/2007   Adapt for EPM...

--
-- Schema for table 'article'
--
-- This table lists the available articles for each application.
-- Articles correspond roughly to FAQs, HOWTOs, and news announcements,
-- and they can be searched.
--

CREATE TABLE article (
  id INTEGER PRIMARY KEY AUTO_INCREMENT,-- Article number
  is_published INTEGER,			-- 0 = private, 1 = public
  section VARCHAR(255),			-- Section of article
  title VARCHAR(255),			-- Title of article
  abstract VARCHAR(255),		-- Plain text abstract of article
  contents TEXT,			-- Contents of article
  create_date INTEGER,			-- Time/date of creation
  create_user VARCHAR(255),		-- User that created the article
  modify_date INTEGER,			-- Time/date of last change
  modify_user VARCHAR(255)		-- User that made the last change
);


--
-- Schema for table 'carboncopy'
--
-- This table tracks users that want to be notified when a resource is
-- modified.  Resources are tracked by filename/URL...
--
-- This is used to notify users whenever a STR or article is updated.
--

CREATE TABLE carboncopy (
  id INTEGER PRIMARY KEY AUTO_INCREMENT,-- Carbon copy ID
  url VARCHAR(255),			-- File or URL
  email VARCHAR(255)			-- Email address
);


--
-- Schema for table 'comment'
--
-- This table tracks comments that are added to a page on the web site.
-- Comments are associated with a specific URL, so you can make comments
-- on any page on the site...
--

CREATE TABLE comment (
  id INTEGER PRIMARY KEY AUTO_INCREMENT,-- Comment ID number
  parent_id INTEGER,			-- Parent comment ID number (reply-to)
  status INTEGER,			-- Moderation status, 0 = dead to 5 = great
  url VARCHAR(255),			-- File/link this comment applies to
  contents text,			-- Comment message
  create_date INTEGER,			-- Date the comment was posted
  create_user VARCHAR(255)		-- Author name/email
);


--
-- Schema for table 'str'
--
-- This table stores software trouble reports.
--

CREATE TABLE str (
  id INTEGER PRIMARY KEY AUTO_INCREMENT,-- STR number
  master_id INTEGER,			-- "Duplicate of" number
  is_published INTEGER,			-- 0 = private, 1 = public
  status INTEGER,			-- 1 = closed/resolved,
					-- 2 = closed/unresolved,
					-- 3 = active, 4 = pending, 5 = new
  priority INTEGER,			-- 1 = rfe, 2 = low, 3 = moderate,
					-- 4 = high, 5 = critical
  scope INTEGER,			-- 1 = unit, 2 = function, 3 = software
  summary text,				-- Plain text summary
  subsystem VARCHAR(255),		-- Subsystem name
  str_version VARCHAR(16),		-- Software version for STR
  fix_version VARCHAR(16),		-- Software version for fix
  manager_email VARCHAR(255),		-- Manager of STR
  create_date INTEGER,			-- Time/date of creation
  create_user VARCHAR(255),		-- User that created the STR
  modify_date INTEGER,			-- Time/date of last change
  modify_user VARCHAR(255)		-- User that made the last change
);


--
-- Schema for table 'strfile'
--
-- This table tracks the files that are attached to a STR.
--

CREATE TABLE strfile (
  id INTEGER PRIMARY KEY AUTO_INCREMENT,-- File ID
  str_id INTEGER,			-- STR number
  is_published INTEGER,			-- 0 = private, 1 = public
  filename VARCHAR(255),		-- Name of file
  create_date INTEGER,			-- Time/date of creation
  create_user VARCHAR(255)		-- User that posted the file
);


--
-- Schema for table 'strtext'
--
-- This table tracks the text messages that are attached to a STR.
--

CREATE TABLE strtext (
  id INTEGER PRIMARY KEY AUTO_INCREMENT,-- Text ID
  str_id INTEGER,			-- STR number
  is_published INTEGER,			-- 0 = private, 1 = public
  contents TEXT,			-- Text message
  create_date INTEGER,			-- Time/date of creation
  create_user VARCHAR(255)		-- User that posted the text
);


--
-- Schema for table 'users'
--
-- This table lists the users that work on EPM.  Various pages use
-- this table when doing login/logout stuff and when listing the available
-- users to assign stuff to.
--

CREATE TABLE users (
  id INTEGER PRIMARY KEY AUTO_INCREMENT,-- ID
  is_published INTEGER,			-- 0 = private, 1 = public
  name VARCHAR(255),			-- Login name
  email VARCHAR(255),			-- Name/email address
  hash CHAR(32),			-- MD5 hash of name:password
  level INTEGER,			-- 0 = normal user, 100 = admin user
  create_date INTEGER,			-- Time/date of creation
  create_user VARCHAR(255),		-- User that created the user
  modify_date INTEGER,			-- Time/date of last change
  modify_user VARCHAR(255)		-- User that made the last change
);


--
-- End of "$Id: mxml.sql 150 2004-05-21 03:59:17Z mike $".
--
