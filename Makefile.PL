use strict;
use warnings;
use ExtUtils::MakeMaker;

my $XSFILE = 'Store.xs';
my $HDR = 'hreg.h';
my @modules = qw(hr_pl hr_hrimpl hr_implattr hreg);
my $modstring = join(".o ", @modules) . ".o";

my $GENERATED_FILES = "*.o Store.* INLINE.h";

WriteMakefile(
    NAME                => 'Ref::Store',
    AUTHOR              => q{M. Nunberg, <mnunberg@haskalah.org>},
    VERSION_FROM        => 'lib/Ref/Store.pm',
    ABSTRACT_FROM       => 'lib/Ref/Store.pm',
    CONFIGURE_REQUIRES  => { 'InlineX::C2XS' => 0 },
    ($ExtUtils::MakeMaker::VERSION >= 6.3002
      ? ('LICENSE'=> 'perl')
      : ()),
    PL_FILES            => {},
    PREREQ_PM => {
        'Test::More' => 0,
        'Variable::Magic' => 0.43,
        'Task::Weaken'  => 0,
        'Log::Fu'       => 0.05
        
    },
#    LIBS                => ['-lprofiler'],
    OBJECT             => join(".o ", @modules) . ".o Store.o",
    dist                => { COMPRESS => 'gzip -9f', SUFFIX => 'gz', },
    clean               => { FILES => 'Ref-Store-* '. $GENERATED_FILES },
);



sub MY::postamble {
    my $self = shift;
    my $perl = $self->{PERLRUN} ? '$(PERLRUN)' : '$(PERL)';
    my $package_name = 'Ref::Store::XS::cfunc';
    my $module_name = 'Ref::Store';
    #c2xs($module_name, $package_name, ".", { SRC_LOCATION => $HDR, AUTOWRAP => 1});
    return "$XSFILE: $HDR $modstring\n\t$perl genxs.pl $HDR $module_name $package_name"
}