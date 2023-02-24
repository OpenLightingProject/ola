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
    olad/www/light_bulb_off.png \
    olad/www/light_bulb.png \
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
    olad/www/new/index.html
dist_views_DATA = \
    olad/www/new/views/overview.html \
    olad/www/new/views/plugin-info.html \
    olad/www/new/views/plugins.html \
    olad/www/new/views/universe-add.html \
    olad/www/new/views/universe-faders.html \
    olad/www/new/views/universe-header.html \
    olad/www/new/views/universe-keypad.html \
    olad/www/new/views/universe-overview.html \
    olad/www/new/views/universe-patch.html \
    olad/www/new/views/universe-rdm.html \
    olad/www/new/views/universe-settings.html \
    olad/www/new/views/universes.html
dist_js_DATA = \
    olad/www/new/js/app.min.js \
    olad/www/new/js/app.min.js.map
dist_css_DATA = \
    olad/www/new/css/style.min.css
dist_img_DATA = \
    olad/www/new/img/light_bulb_off.png \
    olad/www/new/img/light_bulb.png \
    olad/www/new/img/logo-mini.png \
    olad/www/new/img/logo.png
dist_jquery_DATA = \
    olad/www/new/libs/jquery/js/jquery.min.js
dist_angularroute_DATA = \
    olad/www/new/libs/angular-route/js/angular-route.min.js
dist_angular_DATA = \
    olad/www/new/libs/angular/js/angular.min.js
dist_bootjs_DATA = \
    olad/www/new/libs/bootstrap/js/bootstrap.min.js
dist_bootfonts_DATA = \
    olad/www/new/libs/bootstrap/fonts/glyphicons-halflings-regular.eot \
    olad/www/new/libs/bootstrap/fonts/glyphicons-halflings-regular.svg \
    olad/www/new/libs/bootstrap/fonts/glyphicons-halflings-regular.ttf \
    olad/www/new/libs/bootstrap/fonts/glyphicons-halflings-regular.woff \
    olad/www/new/libs/bootstrap/fonts/glyphicons-halflings-regular.woff2
dist_bootcss_DATA = \
    olad/www/new/libs/bootstrap/css/bootstrap.min.css
