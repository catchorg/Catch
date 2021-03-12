
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE_1_0.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0
#include <catch2/internal/catch_commandline.hpp>

#include <catch2/catch_config.hpp>
#include <catch2/internal/catch_string_manip.hpp>
#include <catch2/interfaces/catch_interfaces_registry_hub.hpp>
#include <catch2/interfaces/catch_interfaces_reporter_registry.hpp>
#include <catch2/interfaces/catch_interfaces_reporter.hpp>

#include <algorithm>
#include <fstream>
#include <ctime>

namespace Catch {

    Clara::Parser makeCommandLineParser( ConfigData& config ) {

        using namespace Clara;

        auto const setWarning = [&]( std::string const& warning ) {
                auto warningSet = [&]() {
                    if( warning == "NoAssertions" )
                        return WarnAbout::NoAssertions;

                    if ( warning == "NoTests" )
                        return WarnAbout::NoTests;

                    return WarnAbout::Nothing;
                }();

                if (warningSet == WarnAbout::Nothing)
                    return ParserResult::runtimeError( "Unrecognised warning: '" + warning + "'" );
                config.warnings = static_cast<WarnAbout::What>( config.warnings | warningSet );
                return ParserResult::ok( ParseResultType::Matched );
            };
        auto const loadTestNamesFromFile = [&]( std::string const& filename ) {
                std::ifstream f( filename.c_str() );
                if( !f.is_open() )
                    return ParserResult::runtimeError( "Unable to load input file: '" + filename + "'" );

                std::string line;
                while( std::getline( f, line ) ) {
                    line = trim(line);
                    if( !line.empty() && !startsWith( line, '#' ) ) {
                        if( !startsWith( line, '"' ) )
                            line = '"' + line + '"';
                        config.testsOrTags.push_back( line );
                        config.testsOrTags.emplace_back( "," );
                    }
                }
                //Remove comma in the end
                if(!config.testsOrTags.empty())
                    config.testsOrTags.erase( config.testsOrTags.end()-1 );

                return ParserResult::ok( ParseResultType::Matched );
            };
        auto const setTestOrder = [&]( std::string const& order ) {
                if( startsWith( "declared", order ) )
                    config.runOrder = TestRunOrder::Declared;
                else if( startsWith( "lexical", order ) )
                    config.runOrder = TestRunOrder::LexicographicallySorted;
                else if( startsWith( "random", order ) )
                    config.runOrder = TestRunOrder::Randomized;
                else
                    return ParserResult::runtimeError( "Unrecognised ordering: '" + order + "'" );
                return ParserResult::ok( ParseResultType::Matched );
            };
        auto const setRngSeed = [&]( std::string const& seed ) {
                if( seed != "time" )
                    return Clara::Detail::convertInto( seed, config.rngSeed );
                config.rngSeed = static_cast<unsigned int>( std::time(nullptr) );
                return ParserResult::ok( ParseResultType::Matched );
            };
        auto const setColourUsage = [&]( std::string const& useColour ) {
                    auto mode = toLower( useColour );

                    if( mode == "yes" )
                        config.useColour = UseColour::Yes;
                    else if( mode == "no" )
                        config.useColour = UseColour::No;
                    else if( mode == "auto" )
                        config.useColour = UseColour::Auto;
                    else
                        return ParserResult::runtimeError( "colour mode must be one of: auto, yes or no. '" + useColour + "' not recognised" );
                return ParserResult::ok( ParseResultType::Matched );
            };
        auto const setWaitForKeypress = [&]( std::string const& keypress ) {
                auto keypressLc = toLower( keypress );
                if (keypressLc == "never")
                    config.waitForKeypress = WaitForKeypress::Never;
                else if( keypressLc == "start" )
                    config.waitForKeypress = WaitForKeypress::BeforeStart;
                else if( keypressLc == "exit" )
                    config.waitForKeypress = WaitForKeypress::BeforeExit;
                else if( keypressLc == "both" )
                    config.waitForKeypress = WaitForKeypress::BeforeStartAndExit;
                else
                    return ParserResult::runtimeError( "keypress argument must be one of: never, start, exit or both. '" + keypress + "' not recognised" );
            return ParserResult::ok( ParseResultType::Matched );
            };
        auto const setVerbosity = [&]( std::string const& verbosity ) {
            auto lcVerbosity = toLower( verbosity );
            if( lcVerbosity == "quiet" )
                config.verbosity = Verbosity::Quiet;
            else if( lcVerbosity == "normal" )
                config.verbosity = Verbosity::Normal;
            else if( lcVerbosity == "high" )
                config.verbosity = Verbosity::High;
            else
                return ParserResult::runtimeError( "Unrecognised verbosity, '" + verbosity + "'" );
            return ParserResult::ok( ParseResultType::Matched );
        };
        auto const setReporter = [&]( std::string const& reporterSpec ) {
            IReporterRegistry::FactoryMap const& factories = getRegistryHub().getReporterRegistry().getFactories();

            // clear the default reporter
            if (!config._nonDefaultReporterSpecifications) {
                config.reporterSpecifications.clear();
                config._nonDefaultReporterSpecifications = true;
            }

            // Exactly one of the reporters may be specified without an output
            // file, in which case it defaults to the output specified by "-o"
            // (or standard output).
            auto colon = std::find(reporterSpec.begin(), reporterSpec.end(), ':');
            const bool containsFileName = colon != reporterSpec.end();
            std::string reporterName;
            Option<std::string> outputFileName;
            if (!containsFileName) {
                reporterName = reporterSpec;
            } else {
                reporterName = reporterSpec.substr(0, colon-reporterSpec.begin());
                outputFileName = reporterSpec.substr(colon-reporterSpec.begin()+1, reporterSpec.size());
            }

            auto lcReporter = toLower( reporterName );
            auto result = factories.find( lcReporter );

            if( result == factories.end() )
                return ParserResult::runtimeError( "Unrecognized reporter, '" + reporterName + "'. Check available with --list-reporters" );
            if( containsFileName && outputFileName->empty() )
                return ParserResult::runtimeError( "Reporter '" + reporterName + "' has empty filename specified as its output. Supply a filename or remove the colon to use the default output." );

            config.reporterSpecifications.push_back({ std::move(reporterName), std::move(outputFileName) });

            // It would be enough to check this only once at the very end, but there is
            // not a place where we could call this check, so do it every time it could fail.
            // For valid inputs, this is still called at most once.
            if (!containsFileName) {
                int n_reporters_without_file = 0;
                for (auto const& spec : config.reporterSpecifications) {
                    if (spec.outputFileName.none()) {
                        n_reporters_without_file++;
                    }
                }
                if (n_reporters_without_file > 1) {
                    return ParserResult::runtimeError( "Only one reporter may have unspecified output file." );
                }
            }

            return ParserResult::ok( ParseResultType::Matched );
        };

        auto cli
            = ExeName( config.processName )
            | Help( config.showHelp )
            | Opt( config.listTests )
                ["-l"]["--list-tests"]
                ( "list all/matching test cases" )
            | Opt( config.listTags )
                ["-t"]["--list-tags"]
                ( "list all/matching tags" )
            | Opt( config.showSuccessfulTests )
                ["-s"]["--success"]
                ( "include successful tests in output" )
            | Opt( config.shouldDebugBreak )
                ["-b"]["--break"]
                ( "break into debugger on failure" )
            | Opt( config.noThrow )
                ["-e"]["--nothrow"]
                ( "skip exception tests" )
            | Opt( config.showInvisibles )
                ["-i"]["--invisibles"]
                ( "show invisibles (tabs, newlines)" )
            | Opt( config.defaultOutputFilename, "filename" )
                ["-o"]["--out"]
                ( "default output filename" )
            | Opt( accept_many, setReporter, "name[:output-file]" )
                ["-r"]["--reporter"]
                ( "reporter to use (defaults to console)" )
            | Opt( config.name, "name" )
                ["-n"]["--name"]
                ( "suite name" )
            | Opt( [&]( bool ){ config.abortAfter = 1; } )
                ["-a"]["--abort"]
                ( "abort at first failure" )
            | Opt( [&]( int x ){ config.abortAfter = x; }, "no. failures" )
                ["-x"]["--abortx"]
                ( "abort after x failures" )
            | Opt( setWarning, "warning name" )
                ["-w"]["--warn"]
                ( "enable warnings" )
            | Opt( [&]( bool flag ) { config.showDurations = flag ? ShowDurations::Always : ShowDurations::Never; }, "yes|no" )
                ["-d"]["--durations"]
                ( "show test durations" )
            | Opt( config.minDuration, "seconds" )
                ["-D"]["--min-duration"]
                ( "show test durations for tests taking at least the given number of seconds" )
            | Opt( loadTestNamesFromFile, "filename" )
                ["-f"]["--input-file"]
                ( "load test names to run from a file" )
            | Opt( config.filenamesAsTags )
                ["-#"]["--filenames-as-tags"]
                ( "adds a tag for the filename" )
            | Opt( config.sectionsToRun, "section name" )
                ["-c"]["--section"]
                ( "specify section to run" )
            | Opt( setVerbosity, "quiet|normal|high" )
                ["-v"]["--verbosity"]
                ( "set output verbosity" )
            | Opt( config.listReporters )
                ["--list-reporters"]
                ( "list all reporters" )
            | Opt( setTestOrder, "decl|lex|rand" )
                ["--order"]
                ( "test case order (defaults to decl)" )
            | Opt( setRngSeed, "'time'|number" )
                ["--rng-seed"]
                ( "set a specific seed for random numbers" )
            | Opt( setColourUsage, "yes|no" )
                ["--use-colour"]
                ( "should output be colourised" )
            | Opt( config.libIdentify )
                ["--libidentify"]
                ( "report name and version according to libidentify standard" )
            | Opt( setWaitForKeypress, "never|start|exit|both" )
                ["--wait-for-keypress"]
                ( "waits for a keypress before exiting" )
            | Opt( config.benchmarkSamples, "samples" )
                ["--benchmark-samples"]
                ( "number of samples to collect (default: 100)" )
            | Opt( config.benchmarkResamples, "resamples" )
                ["--benchmark-resamples"]
                ( "number of resamples for the bootstrap (default: 100000)" )
            | Opt( config.benchmarkConfidenceInterval, "confidence interval" )
                ["--benchmark-confidence-interval"]
                ( "confidence interval for the bootstrap (between 0 and 1, default: 0.95)" )
            | Opt( config.benchmarkNoAnalysis )
                ["--benchmark-no-analysis"]
                ( "perform only measurements; do not perform any analysis" )
            | Opt( config.benchmarkWarmupTime, "benchmarkWarmupTime" )
                ["--benchmark-warmup-time"]
                ( "amount of time in milliseconds spent on warming up each test (default: 100)" )
            | Arg( config.testsOrTags, "test name|pattern|tags" )
                ( "which test or tests to use" );

        return cli;
    }

} // end namespace Catch
