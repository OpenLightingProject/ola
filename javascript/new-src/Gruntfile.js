/* jslint node: true */
"use strict";
module.exports = function (grunt) {
 grunt.initConfig({
  pkg: grunt.file.readJSON('package.json'),

  bower: {
   dev: {
    options: {
     targetDir: 'javascript/new-src/src/libs',
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
     'olad/www/new/js/app.min.js': ['javascript/new-src/src/js/app.js']
    },
    options: {
     mangle: true,
     sourceMap: true,
     sourceMapName: 'olad/www/new/js/app.min.js.map',
     banner: "/**\n" +
     "* This program is free software; you can redistribute it and/or modify\n" +
     "* it under the terms of the GNU General Public License as published by\n" +
     "* the Free Software Foundation; either version 2 of the License, or\n" +
     "* (at your option) any later version.\n" +
     "*\n" +
     "* This program is distributed in the hope that it will be useful,\n" +
     "* but WITHOUT ANY WARRANTY; without even the implied warranty of\n" +
     "* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n" +
     "* GNU Library General Public License for more details.\n" +
     "*\n" +
     "* You should have received a copy of the GNU General Public License\n" +
     "* along with this program; if not, write to the Free Software\n" +
     "* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.\n" +
     "*/\n"
    }
   }
  },
  jshint: {
   dev: ['javascript/new-src/Gruntfile.js', 'javascript/new-src/src/js/app.js'],
   options: {
    globalstrict: true,
    globals: {
     angular: true
    }
   }
  },
  watch: {
   build: {
    files: ['javascript/new-src/Gruntfile.js', 'javascript/new-src/src/js/app.js', 'javascript/new-src/src/index.html', 'javascript/new-src/src/css/style.css', 'javascript/new-src/src/views/*.html'],
    tasks: ['jshint:dev', 'uglify:default', 'cssmin:default'],
    options: {
     atBegin: true
    }
   }
  },
  cssmin: {
   default: {
    files: {
     'olad/www/new/css/style.min.css': ['javascript/new-src/src/css/style.css']
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

 grunt.file.setBase('../../');

 grunt.registerTask('dev', ['watch:build']);
 grunt.registerTask('build', ['jshint:dev', 'uglify:default', 'cssmin:default']);
};
