wwwdir = $(www_datadir)
newdir = $(www_datadir)/new
viewsdir = $(www_datadir)/new/views
jsdir = $(www_datadir)/new/js
cssdir = $(www_datadir)/new/css
imgdir = $(www_datadir)/new/img
jquerydir = $(www_datadir)/new/libs/jquery/js
angularroutedir = $(www_datadir)/new/libs/angular-route/js
angulardir = $(www_datadir)/new/libs/angular/js
bootcssdir = $(www_datadir)/new/libs/bootstrap/css
bootjsdir = $(www_datadir)/new/libs/bootstrap/js
bootfontsdir = $(www_datadir)/new/libs/bootstrap/fonts

dist_www_DATA = \
				olad/www/back.png \
				olad/www/blank.gif \
				olad/www/button-bg.png \
				olad/www/console_values.html \
				olad/www/custombutton.css \
				olad/www/discovery.png \
				olad/www/editortoolbar.png \
				olad/www/expander.png \
				olad/www/forward.png \
				olad/www/handle.vertical.png \
				olad/www/hide_sections.png \
				olad/www/incremental-discovery.png \
				olad/www/landing.html \
				olad/www/light_bulb.png \
				olad/www/light_bulb_off.png \
				olad/www/loader-mini.gif \
				olad/www/loader.gif \
				olad/www/logo-mini.png \
				olad/www/logo.png \
				olad/www/mobile.html \
				olad/www/mobile.js \
				olad/www/ola.html \
				olad/www/ola.js \
				olad/www/refresh.png \
				olad/www/show_sections.png \
				olad/www/tick.gif \
				olad/www/toolbar-bg.png \
				olad/www/toolbar.css \
				olad/www/toolbar_sprites.png \
				olad/www/vertical.gif \
				olad/www/wand.png \
				olad/www/warning.png
dist_new_DATA = \
    olad/www/index.html
dist_views_DATA = \
    olad/www/views/overview-universe.html \
    olad/www/views/add-universe.html \
    olad/www/views/info-plugins.html \
		olad/www/views/info-plugin.html \
    olad/www/views/overview.html \
    olad/www/views/keypad-universe.html \
    olad/www/views/patch-universe.html \
    olad/www/views/settings-universe.html \
    olad/www/views/sliders-universe.html \
    olad/www/views/overview-universes.html \
    olad/www/views/rdm-universe.html
dist_js_DATA = \
    olad/www/js/app.min.js
dist_css_DATA = \
    olad/www/css/style.min.css
dist_img_DATA = \
    olad/www/img/light_bulb.png \
    olad/www/img/logo.png \
    olad/www/img/light_bulb_off.png \
    olad/www/img/logo-mini.png
dist_jquery_DATA = \
    olad/www/libs/jquery/js/jquery.min.js
dist_angularroute_DATA = \
    olad/www/libs/angular-route/js/angular-route.min.js
dist_angular_DATA = \
    olad/www/libs/angular/js/angular.min.js
dist_bootjs_DATA = \
    olad/www/libs/bootstrap/js/bootstrap.min.js
dist_bootfonts_DATA = \
    olad/www/libs/bootstrap/fonts/glyphicons-halflings-regular.woff \
    olad/www/libs/bootstrap/fonts/glyphicons-halflings-regular.svg \
    olad/www/libs/bootstrap/fonts/glyphicons-halflings-regular.ttf \
    olad/www/libs/bootstrap/fonts/glyphicons-halflings-regular.eot \
    olad/www/libs/bootstrap/fonts/glyphicons-halflings-regular.woff2
dist_bootcss_DATA = \
    olad/www/libs/bootstrap/css/bootstrap.min.css
