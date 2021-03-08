
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE_1_0.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0
#include <catch2/catch_config.hpp>
#include <catch2/internal/catch_enforce.hpp>
#include <catch2/internal/catch_stream.hpp>
#include <catch2/internal/catch_stringref.hpp>
#include <catch2/internal/catch_string_manip.hpp>
#include <catch2/internal/catch_test_spec_parser.hpp>
#include <catch2/interfaces/catch_interfaces_tag_alias_registry.hpp>

#include <ostream>

namespace Catch {
    namespace Detail {
        namespace {
            class RDBufStream : public IStream {
                mutable std::ostream m_os;
            public:
                // Note that the streambuf must live at least as long as this object.
                RDBufStream(std::streambuf *sb) : m_os( sb ) {}
                ~RDBufStream() override = default;

            public: // IStream
                std::ostream& stream() const override { return m_os; }
            };
        } // unnamed namespace
    } // namespace Detail

    std::ostream& operator<<(std::ostream &os, ConfigData::ReporterAndFile const& reporter) {
        os << "{ " << reporter.reporterName;
        if (reporter.outputFileName) {
            os << ", " << *reporter.outputFileName;
        }
        return os << " }";
    }

    Config::Config( ConfigData const& data )
    :   m_data( data ),
        m_defaultStream( openStream(data.defaultOutputFilename) )
    {
        // We need to trim filter specs to avoid trouble with superfluous
        // whitespace (esp. important for bdd macros, as those are manually
        // aligned with whitespace).

        for (auto& elem : m_data.testsOrTags) {
            elem = trim(elem);
        }
        for (auto& elem : m_data.sectionsToRun) {
            elem = trim(elem);
        }

        TestSpecParser parser(ITagAliasRegistry::get());
        if (!m_data.testsOrTags.empty()) {
            m_hasTestFilters = true;
            for (auto const& testOrTags : m_data.testsOrTags) {
                parser.parse(testOrTags);
            }
        }
        m_testSpec = parser.testSpec();

        m_reporterStreams.reserve(m_data.reporterSpecifications.size());
        for (auto const& reporterAndFile : m_data.reporterSpecifications) {
            if (reporterAndFile.outputFileName.none()) {
                m_reporterStreams.emplace_back(new Detail::RDBufStream(m_defaultStream->stream().rdbuf()));
            } else {
                m_reporterStreams.emplace_back(openStream(*reporterAndFile.outputFileName));
            }
        }
    }

    Config::~Config() = default;


    bool Config::listTests() const          { return m_data.listTests; }
    bool Config::listTags() const           { return m_data.listTags; }
    bool Config::listReporters() const      { return m_data.listReporters; }

    std::string Config::getProcessName() const { return m_data.processName; }

    std::vector<std::string> const& Config::getTestsOrTags() const { return m_data.testsOrTags; }
    std::vector<std::string> const& Config::getSectionsToRun() const { return m_data.sectionsToRun; }

    std::vector<ConfigData::ReporterAndFile> const& Config::getReportersAndOutputFiles() const {
        return m_data.reporterSpecifications;
    }

    std::ostream& Config::getReporterOutputStream(std::size_t reporterIdx) const {
        return m_reporterStreams.at(reporterIdx)->stream();
    }

    TestSpec const& Config::testSpec() const { return m_testSpec; }
    bool Config::hasTestFilters() const { return m_hasTestFilters; }

    bool Config::showHelp() const { return m_data.showHelp; }

    // IConfig interface
    bool Config::allowThrows() const                   { return !m_data.noThrow; }
    std::ostream& Config::defaultStream() const        { return m_defaultStream->stream(); }
    std::string Config::name() const                   { return m_data.name.empty() ? m_data.processName : m_data.name; }
    bool Config::includeSuccessfulResults() const      { return m_data.showSuccessfulTests; }
    bool Config::warnAboutMissingAssertions() const    { return !!(m_data.warnings & WarnAbout::NoAssertions); }
    bool Config::warnAboutNoTests() const              { return !!(m_data.warnings & WarnAbout::NoTests); }
    ShowDurations Config::showDurations() const        { return m_data.showDurations; }
    double Config::minDuration() const                 { return m_data.minDuration; }
    TestRunOrder Config::runOrder() const              { return m_data.runOrder; }
    unsigned int Config::rngSeed() const               { return m_data.rngSeed; }
    UseColour Config::useColour() const                { return m_data.useColour; }
    bool Config::shouldDebugBreak() const              { return m_data.shouldDebugBreak; }
    int Config::abortAfter() const                     { return m_data.abortAfter; }
    bool Config::showInvisibles() const                { return m_data.showInvisibles; }
    Verbosity Config::verbosity() const                { return m_data.verbosity; }

    bool Config::benchmarkNoAnalysis() const                      { return m_data.benchmarkNoAnalysis; }
    int Config::benchmarkSamples() const                          { return m_data.benchmarkSamples; }
    double Config::benchmarkConfidenceInterval() const            { return m_data.benchmarkConfidenceInterval; }
    unsigned int Config::benchmarkResamples() const               { return m_data.benchmarkResamples; }
    std::chrono::milliseconds Config::benchmarkWarmupTime() const { return std::chrono::milliseconds(m_data.benchmarkWarmupTime); }

    IStream const* Config::openStream(std::string const& outputFileName) {
        return Catch::makeStream(outputFileName);
    }

} // end namespace Catch
