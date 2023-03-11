/*jshint node: true*/
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
    uglify: {
      build: {
        files: [{
          dest: '../../olad/www/new/js/app.min.js',
          src: [
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
          ]
        }],
        options: {
          mangle: true,
          sourceMap: true,
          sourceMapName: '../../olad/www/new/js/app.min.js.map'
        }
      }
    },
    jshint: {
      dev: [
        'src/app.js',
        'Gruntfile.js',
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
      options: {
        jshintrc: true
      }
    },
    jscs: {
      src: [
        'src/app.js',
        'Gruntfile.js',
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
      options: {
        config: true
      }
    },
    watch: {
      build: {
        files: [
          'Gruntfile.js',
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
          'src/app.js',
          'src/filters/start_form.js',
          'css/style.css'
        ],
        tasks: ['test', 'uglify:build', 'cssmin:build'],
        options: {
          atBegin: true
        }
      }
    },
    cssmin: {
      build: {
        files: [{
          src: 'css/style.css',
          dest: '../../olad/www/new/css/style.min.css'
        }]
      }
    }
  });

  grunt.loadNpmTasks('grunt-contrib-jshint');
  grunt.loadNpmTasks('grunt-contrib-uglify');
  grunt.loadNpmTasks('grunt-contrib-watch');
  grunt.loadNpmTasks('grunt-contrib-cssmin');
  grunt.loadNpmTasks('grunt-bower-task');
  grunt.loadNpmTasks('grunt-jscs');
  grunt.registerTask('dev', ['watch:build']);
  grunt.registerTask('test', ['jshint:dev', 'jscs']);
  grunt.registerTask('build', ['test', 'uglify:build', 'cssmin:build']);
};
