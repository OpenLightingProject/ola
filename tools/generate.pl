#!/usr/bin/perl
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Library General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
# generate.pl
# Generates the messages classes from an xml file
# Copyright (C) 2006 Simon Newton
# 
# This autogenerates a batch of files for RPC to abstract the low level
# structures into objects. The input is an xml file defining the messages.
#
# The following files are generated
#  - module_name_messages.h		the structs
#  - ModuleNameParser.h			the parser interface
#  - ModuleNameParser.cpp		the parser class
#  - ModuleNameMsg.h			the base abstract class for the messages
#  - ModuleNameMsgXXXX.h		one class for each message type
#  - ModuleNameMsgXXXX.cpp		one class for each message type
#  - ModuleNameMsgs.h			includes all the ModuleNameMsgXXXX.h files
#  - Makefile_XXX_YYY.am		the makefile listing the source files generated
#  - ModuleNameCheck.cpp		the testing program
#
# Once complete you can do the following to decode messages:
#
#   UsbProConfParser *p = new UsbProConfParser();
#   UsbProConfMessage *m = p->parse(buffer,length);
#
# Or to send:
#
#  UsbProConfMessage *m = new UsbProConfMessageXX( .. params ..);
#  int len;
#  uint8_t *buf = m->pack(&len);
#
# Bugs:
#  can't handle more than 256 messages
#
# Todo:
#  sequence numbers
#  endianness
#  nested structures
#  create test framework
#
use strict;
use warnings;

my $TMPL_PATH = "./templates";

use XML::Simple;
use Template;


my %TYPES = (
 uint8_t => 1,
 uint16_t => 2,
 uint32_t => 4,
 int8_t => 1,
 int16_t => 2,
 int32_t => 4,
 'char' => 1,
 'int' => 4,
);

main();
exit 0;

sub usage_and_exit {
	print<<END;
Usage: $0 <xml_file>
 Generates the message C++ classes from an XML definitions file
END
	exit 1;
}


sub main {

	usage_and_exit, if($#ARGV < 0);
	
	if ( ! -f $ARGV[0]) {
		print "File not found $ARGV[0]\n";
		exit 1;
	}

	my $xml = XMLin($ARGV[0], ForceArray => 1, KeyAttr => []) ;

	use Data::Dumper ;
#	print Dumper($xml);

	my $vars = parse($xml);
	return if ( !defined($vars) );

	my $dir = $vars->{lib} . '-' . lc($vars->{module}) ;

	if ( ! -d $dir ) {
		mkdir ($dir) or die "Could not create directory:" . $dir;
	}

	my @files = ();
	push @files, output_structs($dir, $vars);
	push @files, output_msg_header($dir, $vars);
	push @files, output_parser_header($dir, $vars);
	push @files, output_parser_class($dir, $vars);
	push @files, output_include_all($dir, $vars);
	push @files, output_subclasses($dir, $vars);
	output_makefile($dir, $vars, \@files);
	output_test_class($dir, $vars);

}

=pod

Turn the parsed xml into a data structure for our template

=cut
sub parse {
	my $xml = shift;

	my %vars;

	if(! defined($xml->{library}) || $xml->{library} !~ /^\w+$/ ) {
		print "Library name not defined or invalid\n";
		return;
	}
	$vars{lib} = $xml->{library};

	if(! defined($xml->{module}) || $xml->{module} !~ /^\w+$/) {
		print "Module not defined or invalid\n";
		return;
	}
	$vars{module} = $xml->{module};

	if(! defined($xml->{name}) || $xml->{name} !~ /^\w+$/) {
		print "Name not defined or invalid\n";
		return;
	}
	$vars{name} = $xml->{name};

	# allows this message family to inherit from another
	$vars{parent} = '';
	if(defined($xml->{parent})) {
		my $parentx = shift @{$xml->{parent}} ;
		print Dumper($parentx) ;
		if ($parentx->{name} && $parentx->{include}) {

			my $parent = {  name => $parentx->{name}->[0],
							include => $parentx->{include}->[0]
						 };
			$vars{parent} = $parent;
		}
	}

	# do we build this set of messages as a library 
	$vars{install} = 0;
	$vars{install} = 1, if(defined($xml->{install}) && $xml->{install});

	# version info for the library (only used with install)
	$vars{version} = $vars{install} ? '0:0:1' : '';
	$vars{version} = $xml->{version}, if defined($xml->{version});


	# static fields (aka proto identifiers)
	$vars{statics} = undef;
	if ($xml->{static}) {
		my @statics = ();
		my $static_size = 0;
		foreach my $statx (@{$xml->{static}}) {
			my $type = $statx->{type} || '';
			if(defined($statx->{value}) && $type && $statx->{name} && defined($TYPES{$type}) ) {
				my %static;
				$static{value} = $statx->{value};
				$static{type} = $statx->{type};
				$static{name} = $statx->{name};
				push @statics, \%static;
				$static_size += $TYPES{$type};
			}
		}
		$vars{statics} = \@statics;
		$vars{statics_size} = $static_size;
	}

print Dumper(%vars) ;

	my @msgs;
	foreach my $msgx (@{$xml->{message}}) {
		my %msg = (
			uint32_t => [],
			uint16_t => [],
			uint8_t => [],
			array => {}
		);
		$msg{name} = $msgx->{name} || '';
		$msg{doc} = $msgx->{doc} || '';
		$msg{cls_name} = $msg{name};
		$msg{cls_name} =~ s/_([a-z])/uc($1)/e;
		$msg{cls_name} =~ s/^([a-z])/uc($1)/e;

		if(defined($msgx->{uint32_t})) {
			foreach my $u32 (@{$msgx->{uint32_t}}) {
				push @{$msg{uint32_t}} , $u32->{name};
			}
		}

		if(defined($msgx->{uint16_t})) {
			foreach my $u16 (@{$msgx->{uint16_t}}) {
				push @{$msg{uint16_t}} , $u16->{name};
			}
		}

		if(defined($msgx->{uint8_t})) {
			foreach my $u8 (@{$msgx->{uint8_t}}) {
				push @{$msg{uint8_t}} , $u8->{name};
			}
		}

		if(defined($msgx->{array})) {
			foreach my $array (@{$msgx->{array}}) {
				my $type = $array->{type};

				unless(defined($msg{array}->{$type})) {
					$msg{array}->{$type} = [];
				}

				push @{$msg{array}->{$type}}, { name => $array->{name}, len =>  $array->{len}};
			}
		}

#		print Dumper(%msg);
		push @msgs, \%msg;
	}

	$vars{msgs} = \@msgs;
	return \%vars;
}

sub output_structs {
	my ($dir, $vars) = @_;
	my $t = Template->new(INCLUDE_PATH => $TMPL_PATH);

	my $file = $dir . '/' . lc($vars->{module}) . '_' . lc($vars->{name}) . '_messages.h';
	$t->process( "structs.tmpl", $vars, $file) || die "Can't output " . $t->error();

	return ($file);
}


sub output_msg_header {
	my ($dir, $vars) = @_;
	my $t = Template->new(INCLUDE_PATH => $TMPL_PATH);

	my $file = $dir. '/' . $vars->{module} . $vars->{name} . 'Msg.h';
	$t->process( "message.h", $vars, $file) || die "Can't output " . $t->error();
	return ($file);
}

sub output_parser_header {
	my ($dir, $vars) = @_;
	my $t = Template->new(INCLUDE_PATH => $TMPL_PATH);

	my $file = $dir. '/' . $vars->{module} . $vars->{name} . 'Parser.h';
	$t->process( "parser.h", $vars, $file) || die "Can't output " . $t->error();
	return ($file);
}

sub output_parser_class {
	my ($dir, $vars) = @_;
	my $t = Template->new(INCLUDE_PATH => $TMPL_PATH);

	my $file = $dir. '/' . $vars->{module} . $vars->{name} . 'Parser.cpp';
	$t->process( "parser.cpp", $vars, $file) || die "Can't output " . $t->error();
	return ($file);
}


sub output_subclasses {
	my ($dir, $vars) = @_;
	my $t = Template->new(INCLUDE_PATH => $TMPL_PATH);
	my @files;

	foreach my $msg (@{$vars->{msgs}}) {
		my %params = ( 	name => $vars->{name},
						lib => $vars->{lib},
						module => $vars->{module},
						cls_name => $msg->{cls_name},
						msg => $msg,
						statics => $vars->{statics},
						statics_size => $vars->{statics_size},
					 );

		my $file = $dir. '/' . $vars->{module} . $vars->{name} . 'Msg' . $msg->{cls_name} . '.cpp';
		$t->process( "sub_class.cpp", \%params, $file) || die "Can't output " . $t->error();
		push @files, $file;

		$file = $dir. '/' . $vars->{module} . $vars->{name} . 'Msg' . $msg->{cls_name} . '.h';
		$t->process( "sub_class.h", \%params, $file) || die "Can't output " . $t->error();
		push @files, $file;
	}
	return @files;
}


sub output_include_all {
	my ($dir, $vars) = @_;
	my $t = Template->new(INCLUDE_PATH => $TMPL_PATH);

	my $file = $dir. '/' . $vars->{module} . $vars->{name} . 'Msgs.h';
	$t->process( "include_all.h", $vars, $file) || die "Can't output " . $t->error();
	return ($file);
}


sub output_makefile {
	my ($dir, $vars, $files) = @_;
	map { s/.*\/(.*?)$/$1/ } @{$files};
	my @files = grep { /\.cpp$/ } @$files;
	my @headers = grep { /\.h$/ } @$files;

	my $t = Template->new(INCLUDE_PATH => $TMPL_PATH);
	$vars->{files} = \@files;
	$vars->{headers} = \@headers;

	my $file = $dir. '/' . 'Makefile_' . $vars->{name} . '_Msg.am';
	$t->process( "Makefile_msg.am", $vars, $file) || die "Can't output " . $t->error();
}


sub output_test_class {
	my ($dir, $vars) = @_;
	my $t = Template->new(INCLUDE_PATH => $TMPL_PATH);

	my $file = $dir. '/' . $vars->{module} . $vars->{name} . 'Test.cpp';
	$t->process( "test.cpp", $vars, $file) || die "Can't output " . $t->error();
	return ();
}

