<?php
	$app['sitename'] = "Blood Frontier";
	$app['siteblurb'] = "It's Bloody Fun!";
	$app['siterelver'] = "v0.80 (Beta 1)";
	$app['sitereldate'] = "24th February 2009";
	$app['sitevideo'] = "http://www.youtube.com/v/DHNXAwVXF8g&amp;color1=0x000000&amp;color2=0x000000&amp;border=0&amp;fs=1&amp;egm=0&amp;showsearch=0&amp;showinfo=0&amp;feature=PlayList&amp;p=E7D81AC7DED5E1E3&amp;index=0&amp;playnext=0";
	$app['sitenotice'] = "We need your <i>HELP</i> to keep making this game as awesome as possible; the Lead Developer, Quinton \"Quin\" Reeves toils away for up to 18 hours a day, <i>every</i> day, working on the project. He has been struggling with ancient hardware for a long time due to financial difficulties. Please <i>DONATE</i> to make sure he can keep doing this important job!<br /><form action=\"https://www.paypal.com/cgi-bin/webscr\" method=\"post\"><input type=\"hidden\" name=\"cmd\" value=\"_s-xclick\"><input type=\"hidden\" name=\"hosted_button_id\" value=\"212900\"><input type=\"image\" src=\"https://www.paypal.com/en_AU/i/btn/btn_donate_LG.gif\" name=\"submit\" alt=\"Donate to Quin\"></form></a>";
	$app['siteinfo'] = "In the distant future, humanity has spread throughout the solar system, to Mars and beyond. A vast communications network bridges from colony to colony, human to machine, and machine to human. This seemingly benign keystone of modern inter-planetary society, however, appears to be the carrier of a mysterious techno-biological plague. Any persons so-connected seem to fall ill and die, only to return as ravenous, sub-human cannibals. You, a machine intelligence, an android, remain unafflicted by this strange phenomenon and have been tasked with destroying the growing hordes of the infected, while, hopefully, locating and stopping the source of the epidemic.";
	$app['sitelogo'] = "/bits/logo.png";
	$app['sitecss'] = "/bits/site.css";
	$app['siteico'] = "/bits/favicon.ico";

	$app['sfproject'] = "bloodfrontier";
	$app['sfgroupid'] = 198419;
	$app['sflogo'] = 11;
	$app['sfpiwik'] = 1; // use SF's piwik with idsite=N

	$app['ircnetwork'] = "irc.freenode.net";
	$app['ircchannel'] = "bloodfrontier";
	$app['ircsetup'] = "bloodfrontier";

	$app['deftarg'] = "home";
	$app['defsearch'] = "Blood+Frontier";

	$app['targets'] = array('home' => array('name' => 'Home', 'url' => '/', 'alturl' => ''));
	$app['targets']['download'] = array('name' => 'Download', 'url' => 'http://sourceforge.net/project/platformdownload.php?group_id='.$app['sfgroupid'], 'alturl' => 'http://sourceforge.net/project/platformdownload.php?group_id='.$app['sfgroupid'].'&amp;sel_platform=');
	$app['targets']['blog'] = array('name' => 'Blog', 'url' => 'http://apps.sourceforge.net/wordpress/'.$app['sfproject'].'/', 'alturl' => 'http://apps.sourceforge.net/wordpress/'.$app['sfproject'].'/');
	$app['targets']['wiki'] = array('name' => 'Wiki', 'url' => 'http://apps.sourceforge.net/mediawiki/'.$app['sfproject'].'/', 'alturl' => 'http://apps.sourceforge.net/mediawiki/'.$app['sfproject'].'/index.php?title=');
	$app['targets']['forums'] = array('name' => 'Forums', 'url' => 'http://apps.sourceforge.net/phpbb/'.$app['sfproject'].'/', 'alturl' => 'http://apps.sourceforge.net/phpbb/'.$app['sfproject'].'/viewforum.php?f=');
	$app['targets']['chat'] = array('name' => 'Chat', 'url' => 'http://embed.mibbit.com/?server='.$app['ircnetwork'].'&amp;channel=%23'.$app['ircchannel'].'&amp;settings='.$app['ircsetup'].'&amp;forcePrompt=true', 'alturl' => '');
	$app['targets']['gallery'] = array('name' => 'Gallery', 'url' => 'http://apps.sourceforge.net/gallery/'.$app['sfproject'].'/', 'alturl' => 'http://apps.sourceforge.net/gallery/'.$app['sfproject'].'/index.php?g2_itemId=');
	$app['targets']['project'] = array('name' => 'Project', 'url' => 'http://sourceforge.net/projects/'.$app['sfproject'].'/', 'alturl' => '');
	$app['targets']['svn'] = array('name' => 'SVN', 'url' => 'http://'.$app['sfproject'].'.svn.sourceforge.net/'.$app['sfproject'].'/', 'alturl' => 'http://'.$app['sfproject'].'.svn.sourceforge.net/'.$app['sfproject'].'/?view=rev&amp;rev=');
	$app['targets']['google'] = array('name' => 'On Google', 'url' => 'http://www.google.com/search?q='.$app['defsearch'], 'alturl' => 'http://www.google.com/search?q='.$app['defsearch'].'+');
	$app['targets']['youtube'] = array('name' => 'On YouTube', 'url' => 'http://www.youtube.com/results?search_query='.$app['defsearch'], 'alturl' => 'http://www.youtube.com/results?search_query='.$app['defsearch'].'+');

	function checkarg($arg = "", $def = "") {
		return isset($_GET[$arg]) && $_GET[$arg] != "" ? $_GET[$arg] : $def;
	}
	
	$app['target'] = checkarg("target", $app['deftarg']);
	if (!isset($app['targets'][$app['target']])) $app['target'] = $app['deftarg'];

	$title = checkarg("title");
	$redir = checkarg("redir", "");
	if ($redir != "") {
		$loc = "http://".$_SERVER['HTTP_HOST']."/".$app['target'];
		if ($title != "") $loc .= "/".$title;
		header("Location: ".$loc);
		exit;
	}
	else {
		$app['url'] = $title != "" ? (
				$app['targets'][$app['target']]['alturl'] != "" ? $app['targets'][$app['target']]['alturl'].$title : $app['targets'][$app['target']]['url'].$title
		) : $app['targets'][$app['target']]['url'];
		$app['navbar'] = ''; // cache the navbar
		foreach ($app['targets'] as $key => $targ) {
			if ($key != "" && $targ['name'] != "") {
				if ($app['navbar'] != "") $app['navbar'] .= ' | ';
				$app['navbar'] .= '<a href="/'.$key.'">'. $targ['name'] .'</a>';
			}
		}
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en" dir="ltr">
	<head>
		<title><?php echo $app['sitename'].": ".($app['targets'][$app['target']]['name'] != "" ? $app['targets'][$app['target']]['name'] : "Welcome"); ?></title>
		<meta http-equiv="Content-Type" content="text/html;charset=utf-8" />
		<link rel="shortcut icon" href="<?php echo $app['siteico']; ?>" />
		<link rel="stylesheet" type="text/css" href="<?php echo $app['sitecss']; ?>" />
<?php if ($app['target'] != 'home') { ?>
		<script type="text/javascript"><!--
			function setblurb() {
				var b = document.getElementById('blurb');
				b.innerHTML = "<i><?php echo $app['siteblurb']; ?></i>";
			}
			function resizeframe() {
				var c = document.getElementById('frame');
				c.style.display = 'inline';
				c.height = document.documentElement.clientHeight - c.offsetTop;
			}
			window.onresize = resizeframe();
		--></script>
<?php } 
	if ($app['sfpiwik'] > 0) { ?>
		<script type="text/javascript"><!--
			var pkBaseURL = (("https:" == document.location.protocol) ? "https://apps.sourceforge.net/piwik/<?php echo $app['sfproject']; ?>/" : "http://apps.sourceforge.net/piwik/<?php echo $app['sfproject']; ?>/");
			document.write(unescape("%3Cscript src='" + pkBaseURL + "piwik.js' type='text/javascript'%3E%3C/script%3E"));
			piwik_action_name = '<?php echo $app['target']; ?>';
			piwik_idsite = 1;
			piwik_url = pkBaseURL + "piwik.php";
			piwik_log(piwik_action_name, piwik_idsite, piwik_url);
		--></script>
		<object><noscript><p><img src="http://apps.sourceforge.net/piwik/<?php echo $app['sfproject']; ?>/piwik.php?idsite=<?php echo $app['sfpiwik']; ?>" alt="piwik"/></p></noscript></object>
<?php } ?>
	</head>
	<body>
		<div id="header">
			<div id="tags">
				<div id="flink">
<?php			if ($app['sflogo'] > 0) { ?>
					<a href="/project">
						<img id="sflogo" src="http://sflogo.sourceforge.net/sflogo.php?group_id=<?php echo $app['sfgroupid']; ?>&amp;type=<?php echo $app['sflogo']; ?>" style="border: none" alt="Get <?php echo $app['sitename']; ?> at SourceForge" />
					</a>
<?php			} ?>
				</div>
				<div id="blurb"><i><?php echo $app['target'] != 'home' ? "Loading Application.." : $app['siteblurb']; ?></i></div>
			</div>
			<div id="logo"><a href="/home"><img id="sitelogo" src="<?php echo $app['sitelogo']; ?>" alt="<?php echo $app['sitename']; ?>" style="border: none" /></a></div>
			<div id="navbar"><?php echo $app['navbar']; ?></div>
			<div id="version"><b>Current Version:</b> <a href="/download"><?php echo $app['siterelver']; ?></a> released <i><?php echo $app['sitereldate']; ?></i></div>
		</div>
		<div id="body" align="center">
<?php	if ($app['target'] != 'home') { ?>
			<iframe id="frame" frameborder="0" scrolling="yes" marginheight="0" marginwidth="0" onload="resizeframe();setblurb();" src="<?php echo $app['url']; ?>">
				<p>Your browser does not support this feature, to continue visit: <a href="<?php echo $app['url']; ?>"><?php echo $app['targets'][$app['target']]['name']; ?></a></p>
			</iframe>
<?php	} else { ?>
			<div id="portal" align="center">
				<p id="supertext" align="center"><a href="/project"><?php echo $app['sitename']; ?></a>, <i><?php echo $app['siteblurb']; ?></i></p>
				<p id="noticetext" align="center"><?php echo $app['sitenotice']; ?></p>
				<p id="video" align="center">
					<object id="flash" type="application/x-shockwave-flash" data="<?php echo $app['sitevideo']; ?>">
						<param name="movie" value="<?php echo $app['sitevideo']; ?>" />
						<param name="allowscriptaccess" value="always" />
						<param name="allowFullScreen" value="true" />
						<embed id="flash" src="<?php echo $app['sitevideo']; ?>" type="application/x-shockwave-flash" allowfullscreen="true"></embed>
					</object>
				</p>
				<p id="subtext" align="center"><?php echo $app['siteinfo']; ?></p>
				<p id="footer" align="center"><a href="/download">Download</a>, <a href="/wiki">Learn More</a>, <a href="/forums">Get Help</a>, or <a href="/chat">Join In</a> today!</p>
			</div>
<?php	} ?>
		</div>
	</body>
</html>
<?php } ?>