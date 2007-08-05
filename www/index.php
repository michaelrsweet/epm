<?php
//
// "$Id$"
//
// Mini-XML home page...
//

include_once "phplib/html.php";

html_header();

?>

<table width="100%" summary="">
<tr><td valign="top" width="25%">

<table class="table" summary="About EPM">
<tr class="header"><th>About EPM</th></tr>
<tr class="data0"><td style="font-size: 80%;">

<p>EPM is an open source UNIX software and file packaging program
that generates distribution archives from a list of files. EPM
provides a complete, cross-platform software distribution solution
for your applications.</p>

<p>ESP generates both native and "portable" script-based
distribution packages complete with installation and removal
scripts and standard install/uninstall GUIs. The installers can
be customized with product logos, "readme" files, and click-wrap
licenses as desired.</p>

<ul class='noindent'>

	<li>Creates software packages that can be distributed on
	disc or over the Internet!</li>

	<li>Supports AIX, Debian GNU/Linux, FreeBSD, HP-UX, IRIX,
	Mac OS X, NetBSD, OpenBSD, Red Hat Linux, Slackware
	Linux, Solaris, and Tru64 UNIX.</li>

	<li>Provided as free software under the GNU General
	Public license.</li>

	<li>The <a href="documentation.php">EPM book</a> teaches you
	how to create your own software packages that can be
	distributed over the Internet and on disc.</li>

</ul>

</td></tr>
</table>

</td><td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td><td valign='top' width='75%'>

<h2><img src='images/epm-home.jpg' align='right' width='225' height='217'
alt='EPM Logo'>Recent Articles &middot;
<a href='articles.php'>Show&nbsp;All</a></h2>

<?

$result = db_query("SELECT * FROM article WHERE is_published = 1 "
	          ."ORDER BY modify_date DESC LIMIT 4");
$count  = db_count($result);

if ($count == 0)
  print("<p>No articles found.</p>\n");
else
{
  while ($row = db_next($result))
  {
    $id          = $row['id'];
    $title       = htmlspecialchars($row['title'], ENT_QUOTES);
    $abstract    = htmlspecialchars($row['abstract'], ENT_QUOTES);
    $create_user = sanitize_email($row['create_user']);
    $date        = date("H:i M d, Y", $row['modify_date']);
    $count       = count_comments("articles.php_L$id");

    if ($count == 1)
      $count .= " comment";
    else
      $count .= " comments";

    print("<h3><a href='articles.php?L$id'>$title</a></h3>\n"
         ."<p><i>$date by $create_user, $count</i><br>$abstract "
	 ."<a href='articles.php?L$id'>Read...</a></p>\n");
  }
}

db_free($result);

?>

</td></tr>
</table>

<? html_footer(); ?>
