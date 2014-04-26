package My::ModuleBuild;

use strict;
use warnings;
use base qw( Module::Build );
use ExtUtils::CBuilder;
use ExtUtils::CChecker;
use Capture::Tiny qw( capture_merged capture );
use File::Spec;
use Data::Dumper qw( Dumper );
use FindBin ();
use Config;
use FindBin ();
use lib File::Spec->catdir($FindBin::Bin, 'inc', 'PkgConfig', 'lib');
use PkgConfig;
use Text::ParseWords qw( shellwords );
use File::Copy qw( copy );
use File::Temp qw( tempdir );

my $cc;
my %types = ( float => 'float', double => 'double' );

sub macro_line ($)
{
  my($code) = @_;
  sprintf("%-78s\\\n", $code);
}

sub new
{
  my($class, %args) = @_;
  $args{c_source} = 'xs';
  $args{requires}->{'Alien::MSYS'} = '0.04' if $^O eq 'MSWin32';
  
  $cc ||= ExtUtils::CChecker->new(
    quiet => 0,
    defines_to => File::Spec->catfile($FindBin::Bin, 'xs', 'ffi_pl_config1.h'),
  );
  
  my $config2_fh = do {
    my $fn = File::Spec->catfile($FindBin::Bin, 'xs', 'ffi_pl_config2.h');
    my $fh;
    open($fh, '>', $fn) or die "unable to write $fn: $!";
    $fh;
  };
  
  $cc->push_include_dirs( File::Spec->catdir($FindBin::Bin, 'xs') );
  
  $class->c_assert('basic_compiler');

  foreach my $header (qw( stdlib stdint sys/types sys/stat unistd alloca dlfcn limits stddef wchar ))
  {
    my $source = $class->c_tests->{header};
    $source =~ s/<>/<$header.h>/;
    $class->c_tests->{"header_$header"} = $source;
    
    my $define = uc $header;
    $define =~ s/\//_/g;
    
    $class->c_try("header_$header",
      define => "HAS_$define\_H",
    );
  }
  
  $class->c_try('int64',
    define => "HAS_INT64_T",
  );
  $class->c_try('uint64',
    define => "HAS_UINT64_T",
  );
  $class->c_try('long_long',
    define => "HAS_LONG_LONG",
  );
  $class->c_try('big_endian',
    define => "HAS_BIG_ENDIAN",
  );
  
  $class->c_assert('basic_int_types');

  do {
    my %r = ();
  
    foreach my $line ($class->c_output)
    {
      if($line =~ /\|(.*?)\|(.*?)\|/)
      {
        $types{$1} = $2;
        $r{$2} = $1 unless defined $r{$2};
      }
    }
    
    my $has_long_double = $class->c_try('long_double',
      define => 'HAS_LONG_DOUBLE',
    );
    
    $types{'long double'} = 'longdouble';
    
    my @other_types = qw(
      dev_t ino_t mode_t nlink_t uid_t gid_t dev_t off_t blksize_t blkcnt_t time_t
      int8_t int16_t int32_t uint8_t uint16_t uint32_t intptr_t uintptr_t
      int_least8_t int_least16_t int_least16_t int_least64_t 
      uint_least8_t uint_least16_t uint_least16_t uint_least64_t 
      ptrdiff_t wchar_t size_t
      wint_t
    );
    
    my $template = $class->c_tests->{type};

    foreach my $type (@other_types)
    {
      my $code = $template;
      $code =~ s{TEST_TYPE}{$type};
      $class->c_tests->{type} = $code;
      $class->c_try('type',
        comment => $type,
      );
      
      foreach my $line ($class->c_output)
      {
        $types{$1} = $2 if $line =~ /\|(.*?)\|(.*?)\|/;
      }
    }
    
    print $config2_fh "#ifndef FFI_PL_CONFIG2_H\n";
    print $config2_fh "#define FFI_PL_CONFIG2_H\n";
    print $config2_fh "\n/* warning, this file is generated by Build.PL (see inc/My/ModuleBuild.pm) */\n\n";
    print $config2_fh   macro_line "#define ffi_pl_sv2ffi(_target, _source, _type)";
    print $config2_fh   macro_line "          switch(_type->ffi_type->type)";
    print $config2_fh   macro_line "          {";
    foreach my $ffi_type (sort keys %r)
    {
      my $c_type = $r{$ffi_type};
      my $SvIV = 'SvIV';
      $SvIV = 'SvI64' if $ffi_type eq 'sint64';
      $SvIV = 'SvU64' if $ffi_type eq 'uint64';
      print $config2_fh macro_line "            case FFI_TYPE_" . sprintf("%-6s", uc($ffi_type)) . ":";
      print $config2_fh macro_line "              *((" . sprintf("%-14s", $c_type) . " *)_target) = $SvIV(_source);";
      print $config2_fh macro_line "              break;";
    }
    print $config2_fh   macro_line "            case FFI_TYPE_FLOAT:";
    print $config2_fh   macro_line "              *((float *)_target) = SvNV(_source);";
    print $config2_fh   macro_line "              break;";
    print $config2_fh   macro_line "            case FFI_TYPE_DOUBLE:";
    print $config2_fh   macro_line "              *((double *)_target) = SvNV(_source);";
    print $config2_fh   macro_line "              break;";
    print $config2_fh              "          }\n\n";  
    
    print $config2_fh   macro_line "#define ffi_pl_str_type2ffi_type(_target, _name)";
    print $config2_fh   macro_line "          if(!strcmp(_name, \"void\"))";
    print $config2_fh   macro_line "          {";
    print $config2_fh   macro_line "            _target->ffi_type = &ffi_type_void;";
    print $config2_fh   macro_line "            _target->name = \"void\";";
    print $config2_fh   macro_line "          }";
    foreach my $ffi_type ((map { ("uint$_", "sint$_") } qw( 8 16 32 64)), qw( double float longdouble ) )
    {
      print $config2_fh macro_line "          else if(!strcmp(_name, \"$ffi_type\"))";
      print $config2_fh macro_line "          {";
      print $config2_fh macro_line "            _target->ffi_type = &ffi_type_$ffi_type;";
      print $config2_fh macro_line "            _target->name = \"$ffi_type\";";
      print $config2_fh macro_line "          }";
    } 
    print $config2_fh   macro_line "          else";
    print $config2_fh   macro_line "          {";
    print $config2_fh   macro_line "            croak(\"No such type: %s\", _name);";
    print $config2_fh   macro_line "            bad = 1;";
    print $config2_fh              "          }\n\n";
    
    print $config2_fh   macro_line "#define ffi_pl_str_c_type2ffi_type(_target, _name)";
    print $config2_fh   macro_line "          if(!strcmp(_name, \"void\"))";
    print $config2_fh   macro_line "          {";
    print $config2_fh   macro_line "            _target->ffi_type = &ffi_type_void;";
    print $config2_fh   macro_line "            _target->name = \"void\";";
    print $config2_fh   macro_line "          }";
    foreach my $c_type (sort keys %types) {
      my $ffi_type = $types{$c_type};
      print $config2_fh macro_line "          else if(!strcmp(_name, \"$c_type\"))";
      print $config2_fh macro_line "          {";
      print $config2_fh macro_line "            _target->ffi_type = &ffi_type_" . $ffi_type . ";";
      print $config2_fh macro_line "            _target->name = \"$c_type\";";
      print $config2_fh macro_line "          }";
    }
    print $config2_fh   macro_line "          else";
    print $config2_fh   macro_line "          {";
    print $config2_fh   macro_line "            croak(\"No such type: %s\", _name);";
    print $config2_fh   macro_line "            bad = 1;";
    print $config2_fh              "          }\n\n";
    
    print $config2_fh "#endif\n";
    
    close $config2_fh;
  };
  
  $class->c_try('alloca',
    define => 'HAS_ALLOCA',
  ) if !(defined $ENV{FFI_PLATYPUS_BUILD_ALLOCA}) || $ENV{FFI_PLATYPUS_BUILD_ALLOCA};

  my $has_system_ffi;
  
  if(!(defined $ENV{FFI_PLATYPUS_BUILD_SYSTEM_FFI}) || $ENV{FFI_PLATYPUS_BUILD_SYSTEM_FFI})
  {
  
    $has_system_ffi = $class->c_try('system_ffi',
      extra_linker_flags => [ '-lffi' ],
      define => 'HAS_SYSTEM_FFI',
      comment => 'try -lffi',
    ) || do {

      my $libs;
      my $cflags;
      my $bad;
      my $ignore;
    
      # tried using ExtUtils::PkgConfig, but it is too noisy on failure
      # and we have other options so don't want to freek peoples out
      ($libs,   $ignore, $bad) = capture { system 'pkg-config', 'libffi', '--libs' };
      ($cflags, $ignore, $bad) = capture { system 'pkg-config', 'libffi', '--cflags' };
  
      $class->c_try('system_ffi',
        extra_linker_flags   => [ shellwords $libs ],
        extra_compiler_flags => [ shellwords $cflags ],
        define => 'HAS_SYSTEM_FFI',
        comment => 'pkg-config',
      ) || do {
    
        my $pkg = PkgConfig->find('libffi');
      
        $class->c_try('system_ffi',
          extra_linker_flags => [$pkg->get_ldflags],
          extra_compiler_flags => [$pkg->get_cflags],
          define => 'HAS_SYSTEM_FFI',
          comment => 'ppkg-config',
        );
        
      };
    };
  }
  
  do {
    my $fn = File::Spec->catfile(qw( xs flag-systemffi.txt ));
    if($has_system_ffi)
    {
      open my $fh, '>', $fn;
      close $fh;
    }
    else
    {
      unlink $fn;
      my $include = File::Spec->catdir(qw( inc libffi include ));
      $include =~ s{\\}{/}g;
      $cc->push_extra_compiler_flags( "-I$include" );

      if($^O eq 'MSWin32' && $Config{cc} !~ /cl(\.exe)?$/)
      {
        my $lib = File::Spec->catfile(qw( inc libffi .libs libffi.lib ));
        $lib =~ s{\\}{/}g;
        $cc->push_extra_linker_flags( $lib );
        $cc->push_extra_compiler_flags( "-DFFI_BUILDING" );
      }
      else
      {
        my $lib = File::Spec->catdir(qw( inc libffi .libs ));
        $lib =~ s{\\}{/}g;
        $cc->push_extra_linker_flags( "-L$lib", '-lffi');
      }
    }
  };
  
  if($^O eq 'MSWin32' && $Config{cc} !~ /cl(\.exe)?$/)
  {
    $cc->push_extra_linker_flags( 'psapi.lib' );
  }
  elsif($^O =~ /^(MSWin32|cygwin)$/)
  {
    $cc->push_extra_linker_flags('-L/usr/lib/w32api') if $^O eq 'cygwin';
    $cc->push_extra_linker_flags('-lpsapi');
  }

  $args{extra_linker_flags} = join ' ', @{ $cc->extra_linker_flags };
  $args{extra_compiler_flags} = join ' ', @{ $cc->extra_compiler_flags };
  
  my $self = $class->SUPER::new(%args);

  $self->add_to_cleanup(
    'build.log',
    '*.core',
    'test-*',
    'xs/ffi_pl_config1.h',
    'xs/ffi_pl_config2.h',
    'testlib/*.o',
    'testlib/*.so',
    'testlib/*.dll',
    'testlib/*.bundle',
    'testlib/ffi_testlib.txt', # TODO: move this to xs dir
    'xs/flag-*.txt',
  );
  
  $self;
}

sub c_assert
{
  my($class, $name, %args) = @_;
  
  $args{die_on_fail} = 1;
  
  $class->c_try($name, %args);
}

my $out;
sub c_output
{
  wantarray ? split /\n/, $out : $out;
}

sub c_try
{
  my($class, $name, %args) = @_;
  my $diag = $name;
  $diag =~ s/_/ /g;
  
  my $ok;
  
  open my $log, '>>', 'build.log';
  print $log "\n\n\n";
  
  print "check $diag ";
  print "($args{comment}) " if $args{comment};
  print $log "check $diag ";
  print $log "($args{comment}) " if $args{comment};
  
  my $source = $class->c_tests->{$name};
  
  $out = capture_merged {
  
    $ok = $cc->try_compile_run(
      diag   => $diag,
      source => $source,
      %args,
    );
  };
  
  if($ok)
  {
    print "ok\n";
    print $log "ok\n$out";
    
    unless($args{no_alter_flags})
    {
      $cc->push_extra_linker_flags(@{ $args{extra_linker_flags} }) if defined $args{extra_linker_flags};
      $cc->push_extra_compiler_flags(@{ $args{extra_compiler_flags} }) if defined $args{extra_compiler_flags};
    }
    
  }
  else
  {
    print $log "fail\n$out\n\n:::\n$source\n:::\n";
    print "fail\n";
    print $out if $args{die_on_fail} || $ENV{FFI_PLATYPUS_BUILD_VERBOSE};
    die "unable to compile" if $args{die_on_fail};
  }
  
  close $log;
  
  $ok;
}

my $_tests;
sub c_tests
{
  return $_tests if $_tests;
  
  $_tests = {};

  my $code = '';
  my $name;
  my @data = <DATA>;
  foreach my $line (@data)
  {
    if($line =~ /^\|(.*)\|$/)
    {
      $_tests->{$name} = $code if defined $name;
      $name = $1;
      $code = '';
    }
    else
    {
      $code .= $line;
    }
  }
  
  $_tests->{$name} = $code;
  
  $_tests;
}

sub ACTION_build
{
  my $self = shift;
  $self->build_testlib unless -r File::Spec->catfile($FindBin::Bin, 'testlib', 'ffi_testlib.txt');
  $self->build_libffi  unless -r File::Spec->catfile($FindBin::Bin, 'xs', 'flag-libffi.txt');
  $self->SUPER::ACTION_build(@_);
}

sub build_libffi
{
  my $self = shift;
  return if -r File::Spec->catfile($FindBin::Bin, 'xs', 'flag-systemffi.txt');
  
  chdir(File::Spec->catdir(qw( inc libffi ))) || die "unable to chdir";
  
  if($^O eq 'MSWin32')
  {
    my $configure_args = 'MAKEILFO=true --disable-builddir --with-pic';
    if($Config{archname} =~ /^MSWin32-x64/)
    {
      $configure_args .= ' --build=x86_64-pc-mingw64';
    }
    if($Config{cc} =~ /cl(\.exe)?$/)
    {
      my $dir = tempdir( CLEANUP => 1 );
      copy('msvcc.sh', "$dir/msvcc.sh");
      $ENV{PATH} = join($Config{path_sep}, $dir, $ENV{PATH});
      $configure_args .= ' --enable-static --disable-shared';
      if ($Config{archname} =~ /^MSWin32-x64/)
      {
        $configure_args .= ' CC="msvcc.sh -m64"';
      }
      else
      {
        $configure_args .= ' CC=msvcc.sh';
      }
      $configure_args .= ' LD=link';
    }
    require Alien::MSYS;
    Alien::MSYS::msys_run("sh configure $configure_args");
    Alien::MSYS::msys_run("make all");
  }
  else
  {
    system qw( ./configure MAKEINFO=true --enable-static --disable-shared --disable-builddir --with-pic );
    die if $?;
    # TODO: should we use gmake if avail?
    system $Config{make}, 'all';
    die if $?;
  }
  
  opendir my $dh, '.libs';
  my @list = readdir $dh;
  closedir $dh;
  
  # remove the dynamic libs just to be sure
  foreach my $file (@list)
  {
    unlink(File::Spec->catfile('.libs', $file))
      if $file =~ /\.so$/ || $file =~ /\.so\./;
  }
  
  chdir(File::Spec->catdir(File::Spec->updir, File::Spec->updir)) || die "unable to chdir";
}

sub build_testlib
{
  my $self = shift;
  
  my $config_fn = File::Spec->catfile($FindBin::Bin, 'testlib', 'ffi_testlib.txt');

  print "Building FFI-Platypus testlib\n";
  
  my $b = ExtUtils::CBuilder->new;
  
  my @c_source = do {
    opendir my $dh, 'testlib';
    my @file_list = readdir $dh;
    closedir $dh;
    grep /\.c$/, @file_list;
  };
  
  my @obj;
  
  foreach my $c_source_file (@c_source)
  {
    push @obj, $b->compile(
      source       => File::Spec->catfile($FindBin::Bin, 'testlib', $c_source_file),
      include_dirs => [
        File::Spec->catdir($FindBin::Bin, 'xs'),
        File::Spec->catdir($FindBin::Bin, 'testlib'),
      ],
    );
  }

  my %config;

  if($^O ne 'MSWin32')
  {
    $config{lib} = $b->link(
      lib_file => $b->lib_file(File::Spec->catfile($FindBin::Bin, 'testlib', 'ffi_testlib.o')),
      objects  => \@obj,
    );
  }
  else
  {
    # On windows we can't depend on MM::CBuilder to make the .dll file because it creates dlls
    # that export only one symbol (which is used for bootstrapping XS modules).
    $config{lib} = File::Spec->catfile($FindBin::Bin, 'testlib', 'ffi_testlib.dll');
    $config{lib} =~ s{\\}{/}g;
    my @cmd;
    if($Config{cc} !~ /cl(\.exe)?$/)
    {
      my $lddlflags = $Config{lddlflags};
      $lddlflags =~ s{\\}{/}g;
      @cmd = ($Config{cc}, shellwords($lddlflags), -o => $config{lib}, "-Wl,--export-all-symbols", @obj);
    }
    else
    {
      @cmd = ($Config{cc}, @obj, '/link', '/dll', '/out:' . $config{lib});
    }
    print "@cmd\n";
    system @cmd;
    exit 2 if $?;
  }
  
  do {
    my $fh;
    open($fh, '>', $config_fn) or die "unable to write $config_fn: $!";
    print $fh Dumper(\%config);
    close $fh;
  };
}

1;

__DATA__
|basic_compiler|
int
main(int argc, char *argv[])
{
  return 0;
}

|basic_int_types|
#include <ffi_pl_type_detect.h>
#include <ffi_pl.h>

int
main(int argc, char *argv[])
{
  print(char);
  print(signed char);
  print(unsigned char);
  print(short);
  print(unsigned short);
  print(int);
  print(unsigned int);
  print(long);
  print(unsigned long);
  print(size_t);
#ifdef HAS_INT64_T
  print(int64_t);
#endif
#ifdef HAS_UINT64_T
  print(uint64_t);
#endif
#ifdef HAS_LONG_LONG
  print(long long);
  print(unsigned long long);
#endif

  /* should be synonyms for various shorts */
  print(signed short);
  print(signed short int);
  print(unsigned short int);
  
  /* should be synonym for int */
  print(signed int);
  
  /* should be synonyms for various longs */
  print(signed long);
  print(signed long int);
  print(unsigned long int);
#ifdef HAS_LONG_LONG
  print(signed long long);
  print(signed long long int);
  print(unsigned long long int);
#endif

  return 0;
}

|type|
#include <ffi_pl_type_detect.h>
#include <ffi_pl.h>

int
main(int args, char *argv[])
{
  print(TEST_TYPE);
  return 0;
}
|int64|
#define HAS_INT64_T
#include <ffi_pl.h>

int
main(int argc, char *argv[])
{
  if(sizeof(int64_t) == 8)
    return 0;
  else
    return 1;
}

|uint64|
#define HAS_INT64_T
#include <ffi_pl.h>

int
main(int argc, char *argv[])
{
  if(sizeof(uint64_t) == 8)
    return 0;
  else
    return 1;
}

|long_long|
int
main(int argc, char *argv[])
{
  long long i;
  return 0;
}

|header|
#include <>
int main(int argc, char *argv[])
{
  return 0;
}

|system_ffi|
#include <ffi.h>

int
main(int argc, char *argv[])
{
  ffi_cif cif;
  ffi_status status;
  ffi_type args[1];
  
  status = ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 0, &ffi_type_void, &args);

  if(status == FFI_OK)
    return 0;
  else
    return 2;
}

|alloca|
#include <ffi_pl.h>

int
main(int argc, char *argv[])
{
  void *ptr;
  size_t size;
  
  size = 512;
  
  ptr = alloca(size);
  
  if(ptr != NULL)
    return 0;
  else
    return 1;
}

|big_endian|
#include <stdio.h>
#include <ffi_pl.h>

/*
  MY_LITTLE_ENDIAN = 0x03020100ul,
  MY_BIG_ENDIAN = 0x00010203ul,
*/

static const union
{
  unsigned char bytes[4];
  uint32_t value;
} host_order = { { 0, 1, 2, 3 } };

int
main(int argc, char *argv[])
{
  if(host_order.value == 0x00010203ul)
  {
    printf("looks like big endian\n");
    return 0;
  }
  else
  {
    printf("is not big endian, guessing little endian then\n");
    return 1;
  }
}

|long_double|
int
main(int argc, char *argv[])
{
  long double var;
  return 0;
}
