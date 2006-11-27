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
#  - ModuleNameMessage.h		the interface for the messages
#  - ModuleNameMessageXXXX.cpp	one class for each message type
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
use strict;
use warnings;

my $TMPL_PATH = "./templates";

use XML::Simple;
use Template;

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

	if ( ! -d $vars->{lib} ) {
		mkdir $vars->{lib} or die "Could not create directory" . $vars->{lib};
	}

	output_structs($vars);
	output_msg_header($vars);
	output_parser_header($vars);
	output_parser_class($vars);
	output_subclasses($vars);

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

		print Dumper(%msg);
		push @msgs, \%msg;
	}

	$vars{msgs} = \@msgs;
	return \%vars;
}

sub output_structs {
	my $vars = shift;
	my $t = Template->new(INCLUDE_PATH => $TMPL_PATH);


	my $file = $vars->{lib}. '/' . lc($vars->{module}) . '_' . lc($vars->{name}) . '_messages.h';
	$t->process( "structs.tmpl", $vars, $file) || die "Can't output " . $t->error();
}


sub output_msg_header {
	my $vars = shift;
	my $t = Template->new(INCLUDE_PATH => $TMPL_PATH);

	my $file = $vars->{lib}. '/' . $vars->{module} . $vars->{name} . 'Msg.h';
	$t->process( "message.h", $vars, $file) || die "Can't output " . $t->error();
}

sub output_parser_header {
	my $vars = shift;
	my $t = Template->new(INCLUDE_PATH => $TMPL_PATH);

	my $file = $vars->{lib}. '/' . $vars->{module} . $vars->{name} . 'Parser.h';
	$t->process( "parser.h", $vars, $file) || die "Can't output " . $t->error();
}

sub output_parser_class {
	my $vars = shift;
	my $t = Template->new(INCLUDE_PATH => $TMPL_PATH);

	my $file = $vars->{lib}. '/' . $vars->{module} . $vars->{name} . 'Parser.cpp';
	$t->process( "parser.cpp", $vars, $file) || die "Can't output " . $t->error();
}


sub output_subclasses {
	my $vars = shift;
	my $t = Template->new(INCLUDE_PATH => $TMPL_PATH);

	foreach my $msg (@{$vars->{msgs}}) {
		my %params = ( 	name => $vars->{name},
						lib => $vars->{lib},
						module => $vars->{module},
						cls_name => $msg->{cls_name},
						msg => $msg
					 );

		my $file = $vars->{lib}. '/' . $vars->{module} . $vars->{name} . 'Msg' . $msg->{cls_name} . '.cpp';
		$t->process( "sub_class.cpp", \%params, $file) || die "Can't output " . $t->error();

		$file = $vars->{lib}. '/' . $vars->{module} . $vars->{name} . 'Msg' . $msg->{cls_name} . '.h';
		$t->process( "sub_class.h", \%params, $file) || die "Can't output " . $t->error();
	}
}
