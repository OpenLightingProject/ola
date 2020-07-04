/*jshint node: true*/
// banner for app.js
var license_banner =
  '\/**\n' +
  ' * This program is free software; you can redistribute it and\/or modify\n' +
  ' * it under the terms of the GNU General Public License as published by\n' +
  ' * the Free Software Foundation; either version 2 of the License, or\n' +
  ' * (at your option) any later version.\n' +
  ' *\n' +
  ' * This program is distributed in the hope that it will be useful,\n' +
  ' * but WITHOUT ANY WARRANTY; without even the implied warranty of\n' +
  ' * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n' +
  ' * GNU Library General Public License for more details.\n' +
  ' *\n' +
  ' * You should have received a copy of the GNU General Public License\n' +
  ' * along with this program; if not, write to the Free Software\n' +
  ' * Foundation, Inc., 51 Franklin Street,' +
  ' Fifth Floor, Boston, MA 02110-1301 USA.\n' +
  ' *\n * The new OLA web UI.\n * Copyright (C) 2015 Dave Olsthoorn\n' +
  ' *\/\n';

// array of source files to concat
var targets = {
  js_src: [
    'src/app.js',
    'src/controllers/menu.js',
    'src/controllers/patch_universe.js',
    'src/controllers/rdm_universe.js',
    'src/controllers/universe.js',
    'src/controllers/fader_universe.js',
    'src/controllers/keypad_universe.js',
    'src/controllers/plugins.js',
    'src/controllers/add_universe.js',
    'src/controllers/plugin_info.js',
    'src/controllers/setting_universe.js',
    'src/controllers/header.js',
    'src/controllers/overview.js',
    'src/constants.js',
    'src/directives/autofocus.js',
    'src/factories/ola.js',
    'src/filters/start_from.js'
  ],
  css: [
    'css/style.css'
  ],
  js_dev: [
    'Gruntfile.js'
  ]
};

// create array with linting targets
targets.linting = targets.js_src.concat(targets.js_dev);

// create array with targets for "watch"
targets.watching = targets.linting.concat(targets.css);

// the grunt configuration
module.exports = function(grunt) {
  'use strict';
  grunt.initConfig({
    pkg: grunt.file.readJSON('package.json'),
    bower: {
      dev: {
        options: {
          targetDir: '../../olad/www/new/libs',
          install: true,
          copy: true,
          cleanup: true,
          layout: 'byComponent'
        }
      }
    },
    concat: {
      options: {
        banner: license_banner,
        stripBanners: {
          block: false
        }
      },
      build: {
        src: targets.js_src,
        dest: '../../olad/www/new/js/app.js'
      }
    },
    uglify: {
      build: {
        files: [{
          dest: '../../olad/www/new/js/app.min.js',
          src: ['../../olad/www/new/js/app.js']
        }],
        options: {
          mangle: true,
          sourceMap: true,
          sourceMapName: '../../olad/www/new/js/app.min.js.map'
        }
      }
    },
    jshint: {
      dev: targets.linting,
      options: {
        jshintrc: true
      }
    },
    jscs: {
      src: targets.linting,
      options: {
        config: true,
        verbose: true
      }
    },
    stylelint: {
      all: [
        'css/*.css',
        '../../olad/www/*.html',
        '../../tools/rdm/static/*.html',
        '../../tools/rdm/static/common.css',
        '../../tools/rdm/static/ui.multiselect.css'
      ]
    },
    watch: {
      build: {
        files: targets.watching,
        tasks: ['test', 'uglify:build', 'cssmin:build'],
        options: {
          atBegin: true
        }
      }
    },
    cssmin: {
      build: {
        files: [{
          src: targets.css,
          dest: '../../olad/www/new/css/style.min.css'
        }]
      }
    }
  });

  grunt.loadNpmTasks('grunt-contrib-jshint');
  grunt.loadNpmTasks('grunt-contrib-uglify');
  grunt.loadNpmTasks('grunt-contrib-watch');
  grunt.loadNpmTasks('grunt-contrib-cssmin');
  grunt.loadNpmTasks('grunt-contrib-concat');
  grunt.loadNpmTasks('grunt-bower-task');
  grunt.loadNpmTasks('grunt-jscs');
  grunt.loadNpmTasks('grunt-stylelint');

  grunt.registerTask('dev', ['watch:build']);
  grunt.registerTask('test', ['jshint:dev', 'jscs', 'stylelint']);
  grunt.registerTask('build:js', ['concat:build', 'uglify:build']);
  grunt.registerTask('build', ['test', 'build:js', 'cssmin:build']);
};
