<?php
//
// "$Id$"
//
// Documentation page...
//

include_once "phplib/html.php";

html_header("Documentation",
            array("EPM Book" => "epm-book.html",
	          "epm(1)" => "epm-book.html#8_1",
	          "epminstall(1)" => "epm-book.html#8_2",
		  "mkepmlist(1)" => "epm-book.html#8_3",
		  "setup(1)" => "epm-book.html#8_4"));

?>

<p><img src="images/epm-book.jpg" align="right" width="258"
height="322" alt="Software Packaging Using the ESP Package
Manager">The EPM book, "Software Distribution Using the ESP Package
Manager", teaches you how to use the ESP Package Manager to create
your own software packages that can be distributed over the Internet
and on traditional media such as CD-ROM and DVD-ROM. It is available
on-line and in print:</p>

<ul>

	<li><a href="http://www.easysw.com/epm/book.php">Printed
	Book</a></li>

	<li><a href="epm-book.html">On-Line Version</a>

	<ul>

		<li><a href="epm-book.html#1">Preface</a></li>

		<li><a href="epm-book.html#INTRO">1 - Introduction
		to EPM</a></li>

		<li><a href="epm-book.html#BUILDING">2 - Building
		EPM</a></li>

		<li><a href="epm-book.html#PACKAGINS">3 - Packaging
		Your Software</a></li>

		<li><a href="epm-book.html#ADVANCED">4 - Advanced
		Packaging with EPM</a></li>

		<li><a href="epm-book.html#EXAMPLES">5 - EPM
		Packaging Examples</a></li>

		<li><a href="epm-book.html#LICENSE">A - GNU General
		Public License</a></li>

		<li><a href="epm-book.html#MANPAGES">B - Command
		Reference</a>
		<ul>

			<li><a href="epm-book.html#8_1">epm(1)</a></li>

			<li><a href="epm-book.html#8_2">epminstall(1)</a></li>

			<li><a href="epm-book.html#8_3">mkepmlist(1)</a></li>

			<li><a href="epm-book.html#8_4">setup(1)</a></li>

		</ul></li>

		<li><a href="epm-book.html#REFERENCE">C - List File
		Reference</a></li>

		<li><a href="epm-book.html#RELNOTES">D - Release
		Notes</a></li>

	</ul></li>

</ul>

<? html_footer(); ?>
