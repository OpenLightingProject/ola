wwwdir = $(www_datadir)
viewsdir = $(www_datadir)/views
jsdir = $(www_datadir)/js
cssdir = $(www_datadir)/css
imgdir = $(www_datadir)/img
jquerydir = $(www_datadir)/libs/jquery/js
angularroutedir = $(www_datadir)/libs/angular-route/js
angulardir = $(www_datadir)/libs/angular/js
bootcssdir = $(www_datadir)/libs/bootstrap/css
bootjsdir = $(www_datadir)/libs/bootstrap/js
bootfontsdir = $(www_datadir)/libs/bootstrap/fonts


dist_www_DATA = \
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
    olad/www/js/app.js \
    olad/www/js/app.min.js \
    olad/www/js/app.min.js.map
dist_css_DATA = \
    olad/www/css/style.css \
    olad/www/css/style.min.css
dist_img_DATA = \
    olad/www/img/light_bulb.png \
    olad/www/img/logo.png \
    olad/www/img/light_bulb_off.png \
    olad/www/img/logo-mini.png
dist_jquery_DATA = \
    olad/www/libs/jquery/js/jquery.min.map \
    olad/www/libs/jquery/js/jquery.min.js \
    olad/www/libs/jquery/js/jquery.js
dist_angularroute_DATA = \
    olad/www/libs/angular-route/js/angular-route.min.js.map \
    olad/www/libs/angular-route/js/angular-route.min.js \
    olad/www/libs/angular-route/js/angular-route.js
dist_angular_DATA = \
    olad/www/libs/angular/js/angular.min.js.map \
    olad/www/libs/angular/js/angular.min.js \
    olad/www/libs/angular/js/angular.js
dist_bootjs_DATA = \
    olad/www/libs/bootstrap/js/bootstrap.min.js \
    olad/www/libs/bootstrap/js/bootstrap.js
dist_bootfonts_DATA = \
    olad/www/libs/bootstrap/fonts/glyphicons-halflings-regular.woff \
    olad/www/libs/bootstrap/fonts/glyphicons-halflings-regular.svg \
    olad/www/libs/bootstrap/fonts/glyphicons-halflings-regular.ttf \
    olad/www/libs/bootstrap/fonts/glyphicons-halflings-regular.eot \
    olad/www/libs/bootstrap/fonts/glyphicons-halflings-regular.woff2
dist_bootcss_DATA = \
    olad/www/libs/bootstrap/css/bootstrap.css.map \
    olad/www/libs/bootstrap/css/bootstrap.min.css \
    olad/www/libs/bootstrap/css/bootstrap.css
