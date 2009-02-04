<?php
	$app['sitename'] = "Blood Frontier";
	$app['siteblurb'] = "It's Bloody Fun!";
	$app['siteportal'] = "<script type=\"text/javascript\" src=\"http://widgets.clearspring.com/o/46928cc51133af17/4988e5f09a59e6a1/46928cc5584aa23a/be671ae0/-cpid/ac7e911cace45b2f/autostart/false/widget.js\"><!-- Animoto Video --></script>";
	$app['siteinfo'] = "In the distant future, humanity has spread throughout the solar system, to Mars and beyond. A vast communications network bridges from colony to colony, human to machine, and machine to human. This seemingly benign keystone of modern inter-planetary society, however, appears to be the carrier of a mysterious techno-biological plague. Any persons so-connected seem to fall ill and die, only to return as ravenous, sub-human cannibals. You, a machine intelligence, an android, remain unafflicted by this strange phenomenon and have been tasked with destroying the growing hordes of the infected, while, hopefully, locating and stopping the source of the epidemic.";
	$app['sitelogo'] = "bits/logo.png";
	$app['sitecss'] = "bits/site.css";
	$app['siteico'] = "bits/favicon.ico";

	$app['sfproject'] = "bloodfrontier";
	$app['sflogo'] = "http://sflogo.sourceforge.net/sflogo.php?group_id=198419&amp;type=3";
	$app['sfpiwik'] = 1; // use SF's piwik with idsite=N

	$app['ircnetwork'] = "irc.freenode.net";
	$app['ircchannel'] = "bloodfrontier";
	$app['ircsetup'] = "bloodfrontier";

	$app['deftarg'] = "home";
	$app['defsearch'] = "Blood+Frontier";

	$app['targs'] = array(
		'home' => array(
			'name' => 'Home', 'url' => '/',
			'alturl' => '',
		),
		'download' => array(
			'name' => 'Download', 'url' => 'http://sourceforge.net/project/platformdownload.php?group_id=198419',
			'alturl' => 'http://sourceforge.net/project/platformdownload.php?group_id=198419&sel_platform=',
		),
		'blog' => array(
			'name' => 'Blog', 'url' => 'http://apps.sourceforge.net/wordpress/'.$app['sfproject'].'/',
			'alturl' => 'http://apps.sourceforge.net/wordpress/'.$app['sfproject'].'/',
		),
		'wiki' => array(
			'name' => 'Wiki', 'url' => 'http://apps.sourceforge.net/mediawiki/'.$app['sfproject'].'/',
			'alturl' => 'http://apps.sourceforge.net/mediawiki/'.$app['sfproject'].'/index.php?title=',
		),
		'forums' => array(
			'name' => 'Forums', 'url' => 'http://apps.sourceforge.net/phpbb/'.$app['sfproject'].'/',
			'alturl' => 'http://apps.sourceforge.net/phpbb/'.$app['sfproject'].'/viewforum.php?f=',
		),
		'chat' => array(
			'name' => 'Chat',
			'url' => 'http://embed.mibbit.com/?server='.$app['ircnetwork'].'&channel=%23'.$app['ircchannel'].'&settings='.$app['ircsetup'].'&forcePrompt=true',
			'alturl' => '',
		),
		'gallery' => array(
			'name' => 'Gallery', 'url' => 'http://apps.sourceforge.net/gallery/'.$app['sfproject'].'/',
			'alturl' => 'http://apps.sourceforge.net/gallery/'.$app['sfproject'].'/index.php?g2_itemId=',
		),
		'project' => array(
			'name' => 'Project', 'url' => 'http://sourceforge.net/projects/'.$app['sfproject'].'/', 'alturl' => '',
		),
		'svn' => array(
			'name' => 'SVN', 'url' => 'http://'.$app['sfproject'].'.svn.sourceforge.net/'.$app['sfproject'].'/',
			'alturl' => 'http://'.$app['sfproject'].'.svn.sourceforge.net/'.$app['sfproject'].'/?view=rev&rev=',
		),
		'google' => array(
			'name' => 'On Google', 'url' => 'http://www.google.com/search?q='.$app['defsearch'],
			'alturl' => 'http://www.google.com/search?q='.$app['defsearch'].'+',
		),
		'youtube' => array(
			'name' => 'On YouTube', 'url' => 'http://www.youtube.com/results?search_query='.$app['defsearch'],
			'alturl' => 'http://www.youtube.com/results?search_query='.$app['defsearch'].'+',
		)
	);

	function checkarg($arg = "", $def = "") {
		return isset($_GET[$arg]) && $_GET[$arg] != "" ? $_GET[$arg] : $def;
	}
	
	$app['navbar'] = ''; // cache the navbar
	foreach ($app['targs'] as $key => $targ) {
		if ($key != "" && $targ['name'] != "") {
			if ($app['navbar'] != "") $app['navbar'] .= ' | ';
			$app['navbar'] .= '<a href="'.$key.'">'. $targ['name'] .'</a>';
		}
	}

	$app['target'] = checkarg("target", $app['deftarg']);
	if (!isset($app['targs'][$app['target']])) $app['target'] = $app['deftarg'];

	$title = checkarg("title");
	$app['url'] = $title != "" ? (
			$app['targs'][$app['target']]['alturl'] != "" ? $app['targs'][$app['target']]['alturl'].$title : $app['targs'][$app['target']]['url'].$title
	) : $app['targs'][$app['target']]['url'];
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en" dir="ltr">
	<head>
		<title><?php echo $app['sitename'].": ".($app['targs'][$app['target']]['name'] != "" ? $app['targs'][$app['target']]['name'] : "Welcome"); ?></title>
		<meta http-equiv="Content-Type" content="text/html;charset=utf-8" />
		<link rel="shortcut icon" href="<?php echo $app['siteico']; ?>" />
		<link rel="stylesheet" type="text/css" href="<?php echo $app['sitecss']; ?>" />
<?php	if ($app['target'] != 'home') { ?>
		<script type="text/javascript">
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
		</script>
<?php	} 
		if ($app['sfpiwik'] > 0) { ?>
		<script type="text/javascript">
			var pkBaseURL = (("https:" == document.location.protocol) ? "https://apps.sourceforge.net/piwik/<?php echo $app['sfproject']; ?>/" : "http://apps.sourceforge.net/piwik/<?php echo $app['sfproject']; ?>/");
			document.write(unescape("%3Cscript src='" + pkBaseURL + "piwik.js' type='text/javascript'%3E%3C/script%3E"));
			piwik_action_name = '<?php echo $app['target']; ?>';
			piwik_idsite = 1;
			piwik_url = pkBaseURL + "piwik.php";
			piwik_log(piwik_action_name, piwik_idsite, piwik_url);
		</script>
		<object><noscript><p><img src="http://apps.sourceforge.net/piwik/<?php echo $app['sfproject']; ?>/piwik.php?idsite=<?php echo $app['sfpiwik']; ?>" alt="piwik"/></p></noscript></object>
<?php	} ?>
	</head>
	<body>
		<div id="header">
			<div id="tags">
				<div id="flink">
					<a href="http://sourceforge.net/">
						<img src="<?php echo $app['sflogo']; ?>" style="border: none" alt="SourceForge.net Logo" />
					</a>
				</div>
				<div id="blurb"><i><?php echo $app['target'] != 'home' ? "Loading: Please Wait" : $app['siteblurb']; ?></i></div>
			</div>
			<div id="logo"><a href="home"><img src="<?php echo $app['sitelogo']; ?>" alt="<?php echo $app['sitename']; ?>" style="border: none" /></a></div>
			<div id="navbar"><?php echo $app['navbar']; ?></div>
		</div>
		<div id="body" align="center">
<?php	if ($app['target'] != 'home') { ?>
			<iframe id="frame" frameborder="0" scrolling="1" marginheight="0" marginwidth="0" onload="resizeframe();setblurb();" src="<?php echo $app['url']; ?>">
				<p>Your browser does not support this feature, to continue visit: <a href="<?php echo $app['url']; ?>"><?php echo $app['targs'][$app['target']]['name']; ?></a></p>
			</iframe>
<?php	} else { ?>
			<div id="portal" align="center">
				<p id="supertext" align="center"><a href="project"><?php echo $app['sitename']; ?></a>, <i><?php echo $app['siteblurb']; ?></i></p>
				<p id="message" align="center"><?php echo $app['siteportal']; ?></p>
				<p id="subtext" align="center"><?php echo $app['siteinfo']; ?></p>
				<p id="footer" align="center"><a href="download">Download</a>, <a href="wiki">Learn More</a>, <a href="forums">Get Help</a>, or <a href="chat">Join In</a> today!</p>
			</div>
<?php	} ?>
		</div>
	</body>
</html>
