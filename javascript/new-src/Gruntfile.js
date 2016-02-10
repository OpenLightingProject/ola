/*jshint node: true*/
// array of source files to concat
var targets = {
  js: [
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
    'src/factories/ola.js',
    'src/filters/start_form.js'
  ],
  css: [
    'css/style.css'
  ],
  js_dev: [
    'Gruntfile.js'
  ]
};

// create array with linting targets
targets.linting = targets.js.concat(targets.js_dev);

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
        stripBanners: true
      },
      build: {
        src: targets.js,
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
  grunt.registerTask('dev', ['watch:build']);
  grunt.registerTask('test', ['jshint:dev', 'jscs']);
  grunt.registerTask('build:js', ['concat:build', 'uglify:build']);
  grunt.registerTask('build', ['test', 'build:js', 'cssmin:build']);
};
