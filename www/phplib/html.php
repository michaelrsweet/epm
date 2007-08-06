<?php
//
// "$Id$"
//
// PHP functions for standardized HTML output...
//
// This file should be included using "include_once"...
//
// Contents:
//
//   html_header()              - Show the standard page header and navbar...
//   html_footer()              - Show the standard footer for a page.
//   html_start_links()         - Start of series of hyperlinks.
//   html_end_links()           - End of series of hyperlinks.
//   html_link()                - Show a single hyperlink.
//   html_links()               - Show an array of links.
//   html_start_box()           - Start a rounded, shaded box.
//   html_end_box()             - End a rounded, shaded box.
//   html_start_table()         - Start a rounded, shaded table.
//   html_end_table()           - End a rounded, shaded table.
//   html_start_row()           - Start a table row.
//   html_end_row()             - End a table row.
//   html_search_words()        - Generate an array of search words.
//   html_select_is_published() - Do a <select> for the "is published" field...
//


//
// Include necessary headers...
//

include_once "globals.php";
include_once "auth.php";
include_once "common.php";


//
// Search keywords...
//

$html_keywords = array(
  "software packaging",
  "software installers",
  "setup",
  "linux",
  "mac os x",
  "unix",
  "solaris",
  "irix",
  "hp-ux",
  "aix",
  "bsd",
  "tru64 unix"
);
$html_path = "";


//
// 'html_header()' - Show the standard page header and navbar...
//

function				// O - User information
html_header($title = "",		// I - Additional document title
            $links = FALSE,		// I - Array of links
            $path = "",			// I - Relative path to root
	    $refresh = "",		// I - Refresh URL
	    $javascript = "")		// I - Javascript for page
{
  global $html_firstlink, $html_keywords, $html_path;
  global $argc, $argv, $PHP_SELF, $LOGIN_USER;


  $html_path = $path;

  // Check for a logout on the command-line...
  if ($argc == 1 && $argv[0] == "logout")
  {
    auth_logout();
    $argc = 0;
  }

  // Common stuff...
  header("Cache-Control: no-cache");

  print("<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.0 Transitional//EN' "
       ."'http://www.w3.org/TR/REC-html40/loose.dtd'>\n");
  print("<html>\n");
  print("<head>\n");

  // Title...
  if ($title != "")
    $html_title = "$title -";
  else
    $html_title = "";

  print("  <title>$html_title ESP Package Manager</title>\n"
       ."  <meta http-equiv='Pragma' content='no-cache'>\n"
       ."  <meta http-equiv='Content-Type' content='text/html; "
       ."charset=utf-8'>\n"
       ."  <link rel='stylesheet' type='text/css' href='{$path}style.css'>\n"
       ."  <link rel='shortcut icon' href='{$path}favicon.ico' "
       ."type='image/x-icon'>\n");

  // If refresh URL is specified, add the META tag...
  if ($refresh != "")
    print("  <meta http-equiv='refresh' content='3; $refresh'>\n");

  // Search engine keywords...
  reset($html_keywords);

  list($key, $val) = each($html_keywords);
  print("<meta name='keywords' content='$val");

  while (list($key, $val) = each($html_keywords))
    print(",$val");

  print("'>\n");

  if ($javascript != "")
    print("  <script type='text/javascript'>\n$javascript\n  </script>\n");

  print("</head>\n"
       ."<body>\n");

  // Standard navigation stuff...
  print("<table width='100%' style='height: 100%' border='0' cellspacing='0' "
       ."cellpadding='0' summary=''>\n"
       ."<tr class='pagetitle'><td><h1>"
       ."<img src='{$path}images/logo.gif' align='left' width='50' "
       ."height='50' alt='EPM'>&nbsp;");

  if ($title != "")
    print($title);
  else
    print("ESP Package Manager");

  print("</h1></td></tr>\n"
       ."<tr class='pageheader'>"
       ."<th align='left' width='100%' nowrap>");

  if ($LOGIN_USER)
    print("<a href='{$path}account.php'>$LOGIN_USER</a>");
  else
    print("<a href='{$path}login.php'>Login</a>");

  print(" &middot; "
       ."<a href='{$path}index.php'>Home</a>"
       ." &middot; "
       ."<a href='{$path}articles.php'>Articles</a>"
       ." &middot; "
       ."<a href='{$path}str.php'>Bugs &amp; Features</a>"
       ." &middot; "
       ."<a href='{$path}documentation.php'>Documentation</a>"
       ." &middot; "
       ."<a href='{$path}software.php'>Download</a>"
       ." &middot; "
       ."<a href='{$path}forums.php'>Forums</a>"
       ."</th></tr>\n");

  if ($links !== FALSE)
  {
    print("<tr class='pagelinks'><td>$title &middot; ");
    $html_firstlink = 1;
    html_links($links);
    print("</td></tr>\n");
  }

  print("<tr><td class='pagecontents'>");
}


//
// 'html_footer()' - Show the standard footer for a page.
//

function
html_footer()
{
  global $html_path;


  print("</td></tr>\n"
       ."<tr class='pagefooter'><td>"
       ."Copyright 1999-2007 by Easy Software Products. This program is free "
       ."software; you can redistribute it and/or modify it under the terms "
       ."of version 2 of the GNU General Public License as published by the "
       ."Free Software Foundation."
       ."</td></tr>\n"
       ."</table>\n"
       ."</body>\n"
       ."</html>\n");
}


//
// 'html_start_links()' - Start of series of hyperlinks.
//

function
html_start_links($center = 0)		// I - 1 for centered, 0 for in-line
{
  global $html_firstlink;

  $html_firstlink = 1;

  if ($center)
    print("<p class='center' align='center'>");
  else
    print("<p>");
}


//
// 'html_end_links()' - End of series of hyperlinks.
//

function
html_end_links()
{
  print("</p>\n");
}


//
// 'html_link()' - Show a single hyperlink.
//

function
html_link($text,			// I - Text for hyperlink
          $link)			// I - URL for hyperlink
{
  global $html_firstlink;

  if ($html_firstlink)
    $html_firstlink = 0;
  else
    print(" &middot; ");

  $safetext = str_replace(" ", "&nbsp;", $text);

  if (($pos = strpos($link, "@@@@")) !== FALSE)
  {
    $jscript = substr($link, $pos + 4);
    $link    = substr($link, 0, $pos);

    print("<a href='$link' onClick='$jscript'>$safetext</a>");
  }
  else
    print("<a href='$link'>$safetext</a>");
}


//
// 'html_links()' - Show an array of links.
//

function
html_links($links)			// I - Associated array of hyperlinks
{
  reset($links);
  while (list($key, $val) = each($links))
    html_link($key, $val);
}


//
// 'html_start_table()' - Start a rounded, shaded table.
//

function
html_start_table($headings,		// I - Array of heading strings
                 $width = "",		// I - Width of table
		 $height = "",		// I - Height of table
		 $rawhtml = FALSE)	// I - Raw HTML headings?
{
  global $html_row, $html_cols;


  print("<br><table class='table'");
  if ($width != "")
    print(" width='$width'");
  if ($height != "")
    print(" style='height: $height'");
  print(" border='0' cellpadding='0' cellspacing='0' summary=''>"
       ."<tr class='header'>");

  $html_row  = 0;
  $html_cols = count($headings);

  reset($headings);
  for ($i = 0; $i < count($headings); $i ++)
  {
    //
    //  Headings can be in the following forms:
    //
    //    Mix and match as needed:
    //
    //    "xxxxxxxx"            -- Just a column heading.
    //    "xxxxxxxx:aa"         -- Heading with align.
    //    "xxxxxxxx::cc"        -- Heading with a colspan.
    //    "xxxxxxxx:::ww"       -- Heading with a width.
    //    "xxxxxxxx::cc:ww"     -- Heading with colspan and width.
    //    "xxxxxxxx:aa:cc:ww"   -- Heading with align, colspan and width.
    //
    //    etc, etc.
    //

    $s_header  = "";
    $s_colspan = "";
    $s_width   = "";
    $s_align   = "";

    if (!$rawhtml)
    {
      if (strstr($headings[$i], ":"))
      {
	$data     = explode(":", $headings[$i]);
	$s_header = $data[0];

	if ($data[1] != "")
          $s_align = "align=$data[1]";

	if ($data[2] > 1)
	{
          $s_colspan = "colspan=$data[2]";

          if ($data[2] > 1)
            $html_cols += $data[2] - 1;
	}

	if ($data[3] > 0)
          $s_width = "width=$data[3]%";
      }
      else
	$s_header = $headings[$i];
    }
    else
      $s_header = $headings[$i];

    if (strlen($s_header))
      print("<th $s_align $s_colspan $s_width>$s_header</th>");
    else
      print("<th $s_colspan $s_width>&nbsp;</th>");
  }

  print("</tr>\n");
}


//
// 'html_end_table()' - End a rounded, shaded table.
//

function
html_end_table()
{
  global $html_cols;

  print("</table>\n");
}


//
// 'html_start_row()' - Start a table row.
//

function
html_start_row($classname = "")		// I - HTML class to use
{
  global $html_row;

  if ($classname == "")
    $classname = "data$html_row";
  else
    $html_row = 1 - $html_row;

  print("<tr class='$classname'>");
}


//
// 'html_end_row()' - End a table row.
//

function
html_end_row()
{
  global $html_row;

  $html_row = 1 - $html_row;

  print("</tr>\n");
}


//
// 'html_search_words()' - Generate an array of search words.
//

function				// O - Array of words
html_search_words($search = "")		// I - Search string
{
  $words = array();
  $temp  = "";
  $len   = strlen($search);

  for ($i = 0; $i < $len; $i ++)
  {
    switch ($search[$i])
    {
      case "\"" :
          if ($temp != "")
	  {
	    $words[sizeof($words)] = db_escape(strtolower($temp));
	    $temp = "";
	  }

	  $i ++;

	  while ($i < $len && $search[$i] != "\"")
	  {
	    $temp .= $search[$i];
	    $i ++;
	  }

	  $words[sizeof($words)] = db_escape(strtolower($temp));
	  $temp = "";
          break;

      case " " :
      case "\t" :
      case "\n" :
          if ($temp != "")
	  {
	    $words[sizeof($words)] = db_escape(strtolower($temp));
	    $temp = "";
	  }
	  break;

      default :
          $temp .= $search[$i];
	  break;
    }
  }

  if ($temp != "")
    $words[sizeof($words)] = db_escape(strtolower($temp));

  return ($words);
}


//
// 'html_select_is_published()' - Do a <select> for the "is published" field...
//

function
html_select_is_published($is_published = 1)
					// I - Default state
{
  print("<select name='is_published'>");
  if ($is_published)
  {
    print("<option value='0'>No</option>");
    print("<option value='1' selected>Yes</option>");
  }
  else
  {
    print("<option value='0' selected>No</option>");
    print("<option value='1'>Yes</option>");
  }
  print("</select>");
}


?>
