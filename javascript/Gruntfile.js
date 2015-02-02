/*jslint node: true */
"use strict";
module.exports = function (grunt) {
 grunt.initConfig({
  pkg: grunt.file.readJSON('package.json'),

  clean: {
   build: ['../olad/www/index.html', '../olad/www/js', '../olad/www/css', '../olad/www/img', '../olad/www/libs', '../olad/www/views']
  },

  copy: {
   build: {
    files: [
    {
     expand: true,
     src: 'src/img/*',
     dest: '../olad/www/img/',
     flatten: true,
     filter: 'isFile'
    },
    {
     expand: true,
     src: 'src/views/*',
     dest: '../olad/www/views/',
     flatten: true,
     filter: 'isFile'
    },
    {
     expand: true,
     cwd: 'src/libs',
     src: '**',
     dest: '../olad/www/libs',
     flatten: false
    },
    {
     expand: false,
     src: 'src/index.html',
     dest: '../olad/www/index.html',
     flatten: true
    }
    ]
   }
  },

  bower: {
   dev: {
    options: {
     targetDir: 'src/libs',
     install: true,
     copy: true,
     cleanup: true,
     layout: 'byComponent'
    }
   }
  },

  uglify: {
   default: {
    files: {
     '../olad/www/js/app.min.js': ['../olad/www/js/app.js']
    },
    options: {
     mangle: true,
     sourceMap: true,
     sourceMapName: '../olad/www/js/app.min.js.map',
     banner: "/**\n"+
     "* This program is free software; you can redistribute it and/or modify\n"+
     "* it under the terms of the GNU General Public License as published by\n"+
     "* the Free Software Foundation; either version 2 of the License, or\n"+
     "* (at your option) any later version.\n"+
     "*\n"+
     "* This program is distributed in the hope that it will be useful,\n"+
     "* but WITHOUT ANY WARRANTY; without even the implied warranty of\n"+
     "* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"+
     "* GNU Library General Public License for more details.\n"+
     "*\n"+
     "* You should have received a copy of the GNU General Public License\n"+
     "* along with this program; if not, write to the Free Software\n"+
     "* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.\n"+
     "*/\n"
    }
   }
  },

  concat: {
   options: {
    separator: '\n'
   },
   js: {
    src: ['src/js/*.js'],
    dest: '../olad/www/js/app.js'
   },
   css: {
    src: ['src/css/*.css'],
    dest: '../olad/www/css/style.css'
   }
  },

  jshint: {
   dev: ['Gruntfile.js', 'src/js/*.js']
  },

  watch: {
   build: {
    files: ['Gruntfile.js', 'src/js/*.js', 'src/*.html', 'src/css/*.css', 'src/views/*.html'],
    tasks: ['jshint:dev', 'copy:build', 'concat:js', 'uglify:default', 'concat:css', 'cssmin:default'],
    options: {
     atBegin: true
    }
   }
  },

  cssmin: {
   default: {
    files: {
     '../olad/www/css/style.min.css': ['../olad/www/css/style.css']
    }
   }
  }
 });

 grunt.loadNpmTasks('grunt-contrib-jshint');
 grunt.loadNpmTasks('grunt-contrib-clean');
 grunt.loadNpmTasks('grunt-contrib-concat');
 grunt.loadNpmTasks('grunt-contrib-uglify');
 grunt.loadNpmTasks('grunt-contrib-watch');
 grunt.loadNpmTasks('grunt-contrib-cssmin');
 grunt.loadNpmTasks('grunt-contrib-copy');
 grunt.loadNpmTasks('grunt-bower-task');

 grunt.registerTask('dev-server', ['bower:dev', 'watch:dev']);
 grunt.registerTask('build-server', ['watch:build']);
 grunt.registerTask('build', ['clean:build', 'copy:build', 'jshint:dev', 'concat:js', 'uglify:default', 'concat:css', 'cssmin:default']);
};
