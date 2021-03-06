/*
 *  VieSched++ Very Long Baseline Interferometry (VLBI) Scheduling Software
 *  Copyright (C) 2018  Matthias Schartner
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "OperationNotes.h"

using namespace VieVS;
using namespace std;

unsigned long OperationNotes::nextId = 0;

OperationNotes::OperationNotes(const std::string &file): VieVS_Object(nextId++){
    of = ofstream(file);
}

void OperationNotes::writeOperationNotes(const Network &network, const std::vector<Source> &sources,
                                         const std::vector<Scan> &scans,
                                         const std::shared_ptr<const ObservingMode> &obsModes,
                                         const boost::property_tree::ptree &xml,
                                         int version,
                                         boost::optional<MultiScheduling::Parameters> multiSchedulingParameters){

    string expName = xml.get("VieSchedpp.general.experimentName","schedule");

    string currentTimeString = xml.get<string>("VieSchedpp.created.time","");

    boost::posix_time::ptime currentTime = TimeSystem::startTime;

    int year = currentTime.date().year();
    int month = currentTime.date().month();
    int day = currentTime.date().day();
    int doy = currentTime.date().day_of_year();

    int maxDoy;
    boost::gregorian::gregorian_calendar::is_leap_year(year) ? maxDoy = 366 : maxDoy = 365;
    short weekday = currentTime.date().day_of_week();

    string wd = util::weekDay2string(weekday);
    string monthStr = util::month2string(month);

    double yearDecimal = year + static_cast<double>(doy) / static_cast<double>(maxDoy);


    of << "\n                     <<<<< " << expName << " >>>>>\n\n";

    of << boost::format("Date of experiment: %4d,%3s,%02d\n") %(TimeSystem::startTime.date().year()) %(TimeSystem::startTime.date().month()) %(TimeSystem::startTime.date().day()) ;
    of << boost::format("Nominal Start Time: %02dh%02d UT\n") %(TimeSystem::startTime.time_of_day().hours()) %(TimeSystem::startTime.time_of_day().minutes());
    of << boost::format("Nominal End Time:   %02dh%02d UT\n") %(TimeSystem::endTime.time_of_day().hours()) %(TimeSystem::endTime.time_of_day().minutes());
    of << boost::format("Duration:           %.1f hr\n") %(TimeSystem::duration/3600.);
    of << "Correlator:         " << xml.get("VieSchedpp.output.correlator","unknown") << "\n\n";

    of << "Participating stations: (" << network.getNSta() << ")\n";
    for (const auto &any:network.getStations()){
        of << boost::format("%-8s    %2s \n") %any.getName() %any.getAlternativeName();
    }
    of << "\n";

    of << "Contact\n";
    of << "=======\n";
    std::string piName = xml.get("VieSchedpp.output.piName","");
    std::string piEmail = xml.get("VieSchedpp.output.piEmail", "");

    std::string contactName = xml.get("VieSchedpp.output.contactName","");
    std::string contactEmail = xml.get("VieSchedpp.output.contactEmail", "");

    std::string schedulerName = xml.get("VieSchedpp.created.name","");
    std::string schedulerEmail = xml.get("VieSchedpp.created.email", "");

    unsigned long nmax = max({piName.size(), contactName.size(), schedulerName.size()});
    string format = (boost::format("%%-%ds    %%s\n") %nmax).str();

    if(!contactName.empty()){
        of << boost::format(format) %contactName %contactEmail;
    }
    if(!schedulerName.empty()){
        of << boost::format(format) %schedulerName %schedulerEmail;
    }
    if(!piName.empty()){
        of << boost::format(format) %piName %piEmail;
    }
    of << "\n";
    of << "Notes: \n";
    of << "===========================================================\n";

    std::string notes = xml.get("VieSchedpp.output.notes","");
    if(!notes.empty()){
        of << boost::replace_all_copy(notes,"\\n","\n") << "\n";
    }

    bool down = false;
    of << "Station down times:\n";
    for(const auto &sta : network.getStations()){
        bool flag = sta.listDownTimes(of);
        down = down || flag;
    }
    if(!down){
        of << "    none\n";
    }

    of << "Tagalong mode used:\n";
    bool tag = false;
    for(const auto &sta : network.getStations()){
        bool flag = sta.listTagalongTimes(of);
        tag = tag || flag;
    }
    if(!tag){
        of << "    none\n";
    }
    of << "\n";
    of << "===========================================================\n";


    of << "Session Notes for session: " << expName << "\n";
    of << "===========================================================\n";
    of << boost::format(" Experiment: %-17s            Description: %-s\n") %expName %(xml.get("VieSchedpp.output.experimentDescription","no_description"));
    of << boost::format(" Scheduler:  %-17s            Correlator:  %-s\n") %(xml.get("VieSchedpp.output.scheduler","unknown")) %(xml.get("VieSchedpp.output.correlator","unknown"));
    of << boost::format(" Start:      %-17s            End:         %-s\n") %(TimeSystem::time2string_doySkdDowntime(0)) %(TimeSystem::time2string_doySkdDowntime(TimeSystem::duration));
    of << boost::format(" Current yyyyddd:    %4d%03d (%7.2f)  ( %5d MJD, %s. %2d%s.)\n") %year %doy %yearDecimal %(currentTime.date().modjulian_day()) %wd %day %monthStr;
    of << "===========================================================\n";
    of << boost::format(" Software:   %-17s            Version:     %-s\n") %"VieSched++" %(util::version().substr(0,7));
    of << boost::format(" GUI:        %-17s            Version:     %-s\n") %"VieSched++" %(xml.get("VieSchedpp.software.GUI_version","unknown").substr(0,7));
    if(!piName.empty()) {
        of << boost::format(" PI:         %-27s  mail:        %-s\n") %piName %piEmail;
    }
    if(!contactName.empty()) {
        of << boost::format(" contact:    %-27s  mail:        %-s\n") %contactName %contactEmail;
    }
    if(!schedulerName.empty()) {
        of << boost::format(" scheduler:  %-27s  mail:        %-s\n") %schedulerName %schedulerEmail;
    }
    of << "===========================================================\n";
    firstLastObservations_skdStyle(expName, network, sources, scans);
    of << "===========================================================\n";

    if(version>0){
        of << " Schedule was created using multi scheduling tool\n";
        of << "    version " << version << "\n";
        multiSchedulingParameters->output(of);
        of << "===========================================================\n";
    }
    listKeys(network);
    displayTimeStatistics(network, obsModes);
    displayBaselineStatistics(network);
    displayNstaStatistics(network, scans);
    of << "===========================================================\n";

    obsModes->operationNotesSummary(of);

    of << "================================================================================================================================================\n";
    of << "                                                         ADDITONAL NOTES FOR SCHEDULER                                                         \n";
    of << "================================================================================================================================================\n\n";

    displayGeneralStatistics(scans);
    WeightFactors::summary(of);

    displaySkyCoverageScore(network);
    displayStationStatistics(network);
    displaySourceStatistics(sources);
    obsModes->summary(of);
    of << "\n";
    displaySNRSummary(network, sources, scans, obsModes);
    displayScanDurationStatistics(network, scans);
    if(scans.size()>=2){
        of << "First Scans:\n";
        of << ".----------------------------------------------------------------------------------------------------------------------------------------------.\n";
        for(unsigned long i=0; i<3; ++i){
            const auto &thisScan = scans[i];
            thisScan.output(i,network,sources[thisScan.getSourceId()],of);
        }
        of << "\n";
        of << "Last Scans:\n";
        of << ".----------------------------------------------------------------------------------------------------------------------------------------------.\n";
        for(unsigned long i= scans.size() - 3; i < scans.size(); ++i){
            const auto &thisScan = scans[i];
            thisScan.output(i,network,sources[thisScan.getSourceId()],of);
        }
        of << "\n";
    }
    displayAstronomicalParameters();

    of << "================================================================================================================================================\n";
    of << "                                                              FULL SCHEDULING SETUP                                                             \n";
    of << "================================================================================================================================================\n";
    of << "Can be used to recreate schedule:\n\n";
    boost::property_tree::xml_parser::write_xml(of, xml,
                                                boost::property_tree::xml_writer_make_settings<std::string>('\t', 1));

}

void OperationNotes::writeSkdsum(const Network &network, const std::vector<Source>& sources, const std::vector<Scan> & scans) {
    {
        displayGeneralStatistics(scans);
        displayBaselineStatistics(network);
        displayStationStatistics(network);
        displaySourceStatistics(sources);
        displayNstaStatistics(network, scans);
        displayScanDurationStatistics(network, scans);
        displayAstronomicalParameters();
    }
}

void  OperationNotes::displayGeneralStatistics(const std::vector<Scan> & scans) {
    auto n_scans = static_cast<int>(scans.size());
    int n_standard = 0;
    int n_highImpact = 0;
    int n_fillin = 0;
    int n_calibrator = 0;
    int n_single = 0;
    int n_subnetting = 0;

    int obs_max = 0;
    int obs = 0;
    int obs_standard = 0;
    int obs_highImpact = 0;
    int obs_fillin = 0;
    int obs_calibrator = 0;
    int obs_single = 0;
    int obs_subnetting = 0;

    for (const auto&any:scans){
        switch (any.getType()){
            case Scan::ScanType::fillin:{
                ++n_fillin;
                obs_fillin += any.getNObs();
                break;
            }
            case Scan::ScanType::calibrator:{
                ++n_calibrator;
                obs_calibrator += any.getNObs();
                break;
            }
            case Scan::ScanType::standard:{
                ++n_standard;
                obs_standard += any.getNObs();
                break;
            }
            case Scan::ScanType::highImpact:{
                ++n_highImpact;
                obs_highImpact += any.getNObs();
                break;}
        }
        switch (any.getScanConstellation()){
            case Scan::ScanConstellation::single:{
                ++n_single;
                obs_single += any.getNObs();
                break;
            }
            case Scan::ScanConstellation::subnetting:{
                ++n_subnetting;
                obs_subnetting += any.getNObs();
                break;
            }
        }
        obs += any.getNObs();
        obs_max += (any.getNSta()*(any.getNSta()-1))/2;
    }

    if(obs_max-obs > 0){
        of << "number of scheduled observations: " << obs << " of " << obs_max;
        int diff = obs_max-obs;
        of << boost::format(" -> %d (%.2f [%%]) observations not optimized for SNR\n") % diff % (static_cast<double>(diff)/static_cast<double>(obs_max)*100);
    }

    of << boost::format("                 #scans     #obs   \n");
    of << "--------------------------------- \n";
    of << boost::format(" total           %6d   %6d  \n") %n_scans %obs;
    of << "--------------------------------- \n";
    of << boost::format(" single source   %6d   %6d  \n") %n_single %obs_single;
    of << boost::format(" subnetting      %6d   %6d  \n") %n_subnetting %obs_subnetting;
    of << "--------------------------------- \n";
    of << boost::format(" standard        %6d   %6d  \n") %n_standard %obs_standard;
    of << boost::format(" fillin mode     %6d   %6d  \n") %n_fillin %obs_fillin;
    if(n_calibrator > 0){
        of << boost::format(" calibrator      %6d   %6d  \n") %n_calibrator %obs_calibrator;
    }
    if(n_highImpact > 0){
        of << boost::format(" high impact     %6d   %6d  \n") %n_highImpact %obs_highImpact;
    }
    of << "\n";


}

void OperationNotes::displayBaselineStatistics(const Network &network) {
    unsigned long nsta = network.getNSta();
    of << "\n      # OF OBSERVATIONS BY BASELINE \n";

    of << "  |";
    for (const auto &any:network.getStations()) {
        of << "  " << any.getAlternativeName() << " ";
    }
    of << " Total\n";
    of << "---";
    for(int i=0; i<network.getNSta(); ++i){
        of << "-----";
    }
    of << "--------\n";

    for (unsigned long staid1 = 0; staid1 < nsta; ++staid1) {
        of <<  network.getStation(staid1).getAlternativeName() << "|";
        for (unsigned long staid2 = 0; staid2 < nsta; ++staid2) {
            if (staid2<staid1+1){
                of << "     ";
            }else{
                unsigned long nBl = network.getBaseline(staid1,staid2).getStatistics().scanStartTimes.size();
                of << boost::format("%4d ") % nBl;
            }
        }
        of << boost::format("%7d\n") % network.getStation(staid1).getNObs();
    }

}

void OperationNotes::firstLastObservations_skdStyle(const string &expName,
                                                    const Network &network,
                                                    const std::vector<Source>& sources,
                                                    const std::vector<Scan> & scans) {
    of << " First observations\n";
    of << " Observation listing from file " << boost::algorithm::to_lower_copy(expName) << ".skd for experiment " << expName << "\n";
    of << " Source      Start      DURATIONS           \n";
    of << " name     yyddd-hhmmss   " ;
    for (const auto &any : network.getStations()){
        of << any.getAlternativeName() << "  ";
    }
    of << "\n";
    vector<char> found(network.getNSta(),false);
    int counter = 0;
    for (const auto &scan : scans){
        of << scan.toSkedOutputTimes(sources[scan.getSourceId()], network.getNSta());
        scan.includesStations(found);
        if (counter > 5 || all_of(found.begin(), found.end(), [](bool v) { return v; })) {
            break;
        }
        ++counter;
    }

    found = vector<char>(network.getNSta(),false);
    of << " Last observations\n";
    unsigned long i = scans.size() - 1;
    counter = 0;
    for ( ; i>=0; --i){
        scans[i].includesStations(found);
        if (counter > 5 || all_of(found.begin(), found.end(), [](bool v) { return v; })) {
            break;
        }
        ++counter;
    }
    for ( ; i<scans.size(); ++i){
        const auto &scan = scans[i];
        of << scan.toSkedOutputTimes(sources[scan.getSourceId()], network.getNSta());
    }
}


void OperationNotes::displayStationStatistics(const Network &network) {

    of << "number of scans per 15 minutes:\n";
    of << ".-------------------------------------------------------------"
          "----------------------------------------------------------------------------.\n";
    of << "|          time since session start (1 char equals 15 minutes)"
          "                                             | #SCANS #OBS |   OBS Time [s] |\n";
    of << "| STATION |0   1   2   3   4   5   6   7   8   9   10  11  12 "
          " 13  14  15  16  17  18  19  20  21  22  23  |             |   sum  average |\n";
    of << "|---------|+---+---+---+---+---+---+---+---+---+---+---+---+--"
          "-+---+---+---+---+---+---+---+---+---+---+---|-------------|----------------|\n";
    for (const auto &thisStation : network.getStations()) {
        of << boost::format("| %8s|") % thisStation.getName();
        const Station::Statistics &stat = thisStation.getStatistics();
        const auto& time_sta = stat.scanStartTimes;
        unsigned int timeStart = 0;
        unsigned int timeEnd = 900;
        for (int j = 0; j < 96; ++j) {
            long c = count_if(time_sta.begin(), time_sta.end(), [timeEnd,timeStart](unsigned int k){
                return k>=timeStart && k<timeEnd ;
            });
            if(c==0){
                of << " ";
            }else if (c<=9){
                of << c;
            }else{
                of << "X";
            }

            timeEnd += 900;
            timeStart += 900;
        }
        of << boost::format("| %6d %4d ") % thisStation.getNTotalScans() % thisStation.getNObs();
        of << boost::format("| %5d %8.1f |\n") % thisStation.getStatistics().totalObservingTime
              % (static_cast<double>(thisStation.getStatistics().totalObservingTime) / static_cast<double>(thisStation.getNTotalScans()));
    }
    of << "'--------------------------------------------------------------"
          "---------------------------------------------------------------------------'\n\n";
}

void OperationNotes::displaySourceStatistics(const std::vector<Source>& sources) {
    of << "number of available sources:   " << sources.size() << "\n";

    long number = count_if(sources.begin(), sources.end(), [](const Source &any){
        return any.getNTotalScans() > 0;
    });

    of << "number of scheduled sources:   " << number << "\n";
    of << "number of scans per 15 minutes:\n";
    of << ".-------------------------------------------------------------"
          "----------------------------------------------------------------------------.\n";
    of << "|          time since session start (1 char equals 15 minutes)"
          "                                             | #SCANS #OBS |   OBS Time [s] |\n";
    of << "|  SOURCE |0   1   2   3   4   5   6   7   8   9   10  11  12 "
          " 13  14  15  16  17  18  19  20  21  22  23  |             |   sum  average |\n";
    of << "|---------|+---+---+---+---+---+---+---+---+---+---+---+---+--"
          "-+---+---+---+---+---+---+---+---+---+---+---|-------------|----------------|\n";
    for (const auto &thisSource : sources) {
        const Source::Statistics &stat = thisSource.getStatistics();
        const auto& time_sta = stat.scanStartTimes;

        if (thisSource.getNObs() == 0) {
            continue;
        }
        of << boost::format("| %8s|") % thisSource.getName();

        unsigned int timeStart = 0;
        unsigned int timeEnd = 900;
        for (int j = 0; j < 96; ++j) {
            long c = count_if(time_sta.begin(), time_sta.end(), [timeEnd,timeStart](unsigned int k){
                return k>=timeStart && k<timeEnd ;
            });
            if(c==0){
                of << " ";
            }else if (c<=9){
                of << c;
            }else{
                of << "X";
            }

            timeEnd += 900;
            timeStart += 900;
        }
        of << boost::format("| %6d %4d ") % thisSource.getNTotalScans() % thisSource.getNObs();
        of << boost::format("| %5d %8.1f |\n") % thisSource.getStatistics().totalObservingTime
              % (static_cast<double>(thisSource.getStatistics().totalObservingTime) / static_cast<double>(thisSource.getNTotalScans()));
    }
    of << "'--------------------------------------------------------------"
          "---------------------------------------------------------------------------'\n\n";
}

void OperationNotes::displayNstaStatistics(const Network &network, const std::vector<Scan> & scans) {
    unsigned long nsta = network.getNSta();
    vector<int> nstas(nsta+1,0);
    int obs = 0;
    int intObs = 0;
    for(const auto &scan:scans){
        ++nstas[scan.getNSta()];
        obs += scan.getNObs();
        for(int i=0; i<scan.getNObs(); ++i){
            const auto &tobs = scan.getObservation(i);
            unsigned long idx1 = *scan.findIdxOfStationId(tobs.getStaid1());
            unsigned long idx2 = *scan.findIdxOfStationId(tobs.getStaid2());
            intObs += scan.getTimes().getObservingDuration(idx1,idx2);
        }
    }

    unsigned long sum = scans.size();

    for(int i=2; i<=nsta; ++i){
        of << boost::format(" Number of %2d-station scans:   %4d (%6.2f %%)\n")%i %nstas[i] %(static_cast<double>(nstas[i])/
                                                                                              static_cast<double>(sum)*100);
    }
    of << boost::format("Total number of scans:    %9d\n") %(scans.size());
    of << boost::format("Total number of obs:      %9d\n") %obs;
    of << boost::format("Total integrated obs-time:%9d\n") %intObs;
    of << boost::format("Average obs-time:         %9.1f\n") % (static_cast<double>(intObs)/obs);
}


void OperationNotes::displayScanDurationStatistics(const Network &network, const std::vector<Scan> & scans) {
    unsigned long nsta = network.getNSta();
    of << "scan observing durations:\n";
    vector<vector<vector<unsigned int>>> bl_durations(nsta,vector<vector<unsigned int>>(nsta));
    vector< unsigned int> maxScanDurations;

    for(const auto&any: scans){
        unsigned long nbl = any.getNObs();
        maxScanDurations.push_back(any.getTimes().getObservingDuration());

        for (int i = 0; i < nbl; ++i) {
            const Observation &obs = any.getObservation(i);
            unsigned long staid1 = obs.getStaid1();
            unsigned long staid2 = obs.getStaid2();
            unsigned int obs_duration = obs.getObservingTime();

            if(staid1<staid2){
                swap(staid1,staid2);
            }
            bl_durations[staid1][staid2].push_back(obs_duration);
        }
    }
    if(maxScanDurations.empty()){
        return;
    }
    unsigned int maxMax = *max_element(maxScanDurations.begin(),maxScanDurations.end());
    maxMax = maxMax/10*10+10;
    unsigned int cache_size = (1+maxMax/100)*10;
    vector<unsigned int> bins;

    unsigned int upper_bound = 0;
    while (upper_bound<maxMax+cache_size){
        bins.push_back(upper_bound);
        upper_bound+=cache_size;
    }

    vector<unsigned int> hist(bins.size()-1,0);
    for (unsigned int any:maxScanDurations) {
        int i = 1;
        while(any>bins[i]){
            ++i;
        }
        ++hist[i-1];
    }

    of << "scan duration:\n";
    for (int i = 0; i < hist.size(); ++i) {
        of << boost::format("%3d-%3d | ") % bins[i] % (bins[i + 1] - 1);
        double percent = 100 * static_cast<double>(hist[i]) / static_cast<double>(maxScanDurations.size());
        percent = round(percent);
        for (int j = 0; j < percent; ++j) {
            of << "+";
        }
        of << "\n";
    }
    of << "\n";

    of << "scheduled scan length:\n";
    of << ".-----------------------------------------------------------------------------------.\n";
    of << "| S1-S2 |  min    10%    50%    90%    95%  97.5%    99%    max   |    sum  average |\n";
    of << "|-------|---------------------------------------------------------|-----------------|\n";

    {
        auto n = static_cast<int>(maxScanDurations.size() - 1);
        sort(maxScanDurations.begin(),maxScanDurations.end());
        of << boost::format("|  ALL  | ");
        of << boost::format("%4d   ") % maxScanDurations[0];
        of << boost::format("%4d   ") % maxScanDurations[n * 0.1];
        of << boost::format("%4d   ") % maxScanDurations[n / 2];
        of << boost::format("%4d   ") % maxScanDurations[n * 0.9];
        of << boost::format("%4d   ") % maxScanDurations[n * 0.95];
        of << boost::format("%4d   ") % maxScanDurations[n * 0.975];
        of << boost::format("%4d   ") % maxScanDurations[n * 0.99];
        of << boost::format("%4d   ") % maxScanDurations[n];
        int sum = accumulate(maxScanDurations.begin(), maxScanDurations.end(), 0);
        double average = static_cast<double>(sum) / (n + 1);
        of << boost::format("| %6d %8.1f |") % sum % average;
        of << "\n";
    }

    of << "|-------|---------------------------------------------------------|-----------------|\n";

    for (unsigned long i = 1; i < nsta; ++i) {
        for (unsigned long j = 0; j < i; ++j) {
            vector<unsigned int>& this_duration = bl_durations[i][j];
            if(this_duration.empty()){
                continue;
            }
            int n = (int) this_duration.size()-1;
            sort(this_duration.begin(),this_duration.end());
            of << boost::format("| %5s | ") % network.getBaseline(i,j).getName();
            of << boost::format("%4d   ") % this_duration[0];
            of << boost::format("%4d   ") % this_duration[n * 0.1];
            of << boost::format("%4d   ") % this_duration[n / 2];
            of << boost::format("%4d   ") % this_duration[n * 0.9];
            of << boost::format("%4d   ") % this_duration[n * 0.95];
            of << boost::format("%4d   ") % this_duration[n * 0.975];
            of << boost::format("%4d   ") % this_duration[n * 0.99];
            of << boost::format("%4d   ") % this_duration[n];
            int sum = accumulate(this_duration.begin(), this_duration.end(), 0);
            double average = static_cast<double>(sum) / (n + 1);
            of << boost::format("| %6d %8.1f |") % sum % average;
            of << "\n";
        }

    }
    of << "'-----------------------------------------------------------------------------------'\n\n";
}

void OperationNotes::displayTimeStatistics(const Network &network, const std::shared_ptr<const ObservingMode> &obsModes) {

    unsigned long nstaTotal = network.getNSta();


    of << "                 ";
    for (const auto &station: network.getStations()) {
        of << boost::format("    %s ") % station.getAlternativeName();
    }
    of << "   Avg\n";

    of << " % obs. time:    ";
    vector<double> obsPer;
    for (const auto &station: network.getStations()) {
        int t = station.getStatistics().totalObservingTime;
        obsPer.push_back(static_cast<double>(t)/static_cast<double>(TimeSystem::duration)*100);
    }
    for(auto p:obsPer){
        of << boost::format("%6.2f ") % p;
    }
    of << boost::format("%6.2f ") % (accumulate(obsPer.begin(),obsPer.end(),0.0)/(network.getNSta()));
    of << "\n";

    of << " % cal. time:    ";
    vector<double> preobPer;
    for (const auto &station: network.getStations()) {
        int t = station.getStatistics().totalPreobTime;
        preobPer.push_back(static_cast<double>(t)/static_cast<double>(TimeSystem::duration)*100);
    }
    for(auto p:preobPer){
        of << boost::format("%6.2f ") % p;
    }
    of << boost::format("%6.2f ") % (accumulate(preobPer.begin(),preobPer.end(),0.0)/(network.getNSta()));
    of << "\n";

    of << " % slew time:    ";
    vector<double> slewPer;
    for (const auto &station: network.getStations()) {
        int t = station.getStatistics().totalSlewTime;
        slewPer.push_back(static_cast<double>(t)/static_cast<double>(TimeSystem::duration)*100);
    }
    for(auto p:slewPer){
        of << boost::format("%6.2f ") % p;
    }
    of << boost::format("%6.2f ") % (accumulate(slewPer.begin(),slewPer.end(),0.0)/(network.getNSta()));
    of << "\n";

    of << " % idle time:    ";
    vector<double> idlePer;
    for (const auto &station: network.getStations()) {
        int t = station.getStatistics().totalIdleTime;
        idlePer.push_back(static_cast<double>(t)/static_cast<double>(TimeSystem::duration)*100);
    }
    for(auto p:idlePer){
        of << boost::format("%6.2f ") % p;
    }
    of << boost::format("%6.2f ") % (accumulate(idlePer.begin(),idlePer.end(),0.0)/(network.getNSta()));
    of << "\n";

    of << " % field system: ";
    vector<double> fieldPer;
    for (const auto &station: network.getStations()) {
        int t = station.getStatistics().totalFieldSystemTime;
        fieldPer.push_back(static_cast<double>(t)/static_cast<double>(TimeSystem::duration)*100);
    }
    for(auto p:fieldPer){
        of << boost::format("%6.2f ") % p;
    }
    of << boost::format("%6.2f ") % (accumulate(fieldPer.begin(),fieldPer.end(),0.0)/(network.getNSta()));
    of << "\n";


    of << " total # scans:  ";
    vector<int> scans;
    for (const auto &station: network.getStations()) {
        scans.push_back(station.getNTotalScans());
    }
    for(auto p:scans){
        of << boost::format("%6d ") % static_cast<double>(p);
    }
    of << boost::format("%6d ") % roundl(accumulate(scans.begin(),scans.end(),0.0)/(network.getNSta()));
    of << "\n";

    of << " # scans/hour:   ";
    vector<double> scansPerH;
    for (const auto &station: network.getStations()) {
        scansPerH.push_back(static_cast<double>(station.getNTotalScans())/(TimeSystem::duration/3600.));
    }
    for(auto p:scansPerH){
        of << boost::format("%6.2f ") % p;
    }
    of << boost::format("%6.2f ") % (accumulate(scansPerH.begin(),scansPerH.end(),0.0)/(network.getNSta()));
    of << "\n";

    of << " total # obs:    ";
    vector<int> obs;
    for (const auto &station: network.getStations()) {
        obs.push_back(station.getNObs());
    }
    for(auto p:obs){
        of << boost::format("%6d ") % static_cast<double>(p);
    }
    of << boost::format("%6d ") % roundl(accumulate(obs.begin(),obs.end(),0.0)/(network.getNSta()));
    of << "\n";

    of << " # obs/hour:     ";
    vector<double> obsPerH;
    for (const auto &station: network.getStations()) {
        obsPerH.push_back(static_cast<double>(station.getNObs())/(TimeSystem::duration/3600.));
    }
    for(auto p:obsPerH){
        of << boost::format("%6.2f ") % p;
    }
    of << boost::format("%6.2f ") % (accumulate(obsPerH.begin(),obsPerH.end(),0.0)/(network.getNSta()));
    of << "\n";


    of << " Avg scan (sec): ";
    vector<double> scanSec;
    for (const auto &station: network.getStations()) {
        scanSec.push_back( static_cast<double>(station.getStatistics().totalObservingTime) / station.getNTotalScans() );
    }
    for(auto p:scanSec){
        of << boost::format("%6.2f ") % p;
    }
    of << boost::format("%6.2f ") % (accumulate(scanSec.begin(),scanSec.end(),0.0)/(network.getNSta()));
    of << "\n";

    if(ObservingMode::type != ObservingMode::Type::simple){
        of << " # Mk5 tracks:   ";
        for (const auto &station: network.getStations()) {
            const auto &tracksBlock = obsModes->getMode(0)->getTracks(station.getId());
            if(tracksBlock.is_initialized()){
                int tracks = tracksBlock.get()->numberOfTracks();
                of << boost::format("%6d ") % tracks;
            }else{
                of << boost::format("%6s ") % "-";
            }

        }
        of << "\n";

        of << " Total TB(M5):   ";
        vector<double> total_tb;
        for (const auto &station: network.getStations()) {
            double obsFreq = obsModes->getMode(0)->recordingRate(station.getId());
            int t = station.getStatistics().totalObservingTime;

            total_tb.push_back(static_cast<double>(t) * obsFreq / (1024*1024*8) );
        }
        for(auto p:total_tb){
            of << boost::format("%6.2f ") % p;
        }
        of << boost::format("%6.2f ") % (accumulate(total_tb.begin(),total_tb.end(),0.0)/(network.getNSta()));
        of << "\n";
    }

}

void OperationNotes::displayAstronomicalParameters() {
    of << ".------------------------------------------.\n";
    of << "| sun position:        | earth velocity:   |\n";
    of << "|----------------------|-------------------|\n";
    of << "| RA:   " << util::ra2dms(AstronomicalParameters::sun_ra[1]) << " " << boost::format("| x: %8.0f [m/s] |\n")%AstronomicalParameters::earth_velocity[0];
    of << "| DEC: "  << util::dc2hms(AstronomicalParameters::sun_dec[1]) << " " << boost::format("| y: %8.0f [m/s] |\n")%AstronomicalParameters::earth_velocity[1];
    of << "|                      " << boost::format("| z: %8.0f [m/s] |\n")%AstronomicalParameters::earth_velocity[2];
    of << "'------------------------------------------'\n\n";

    of << ".--------------------------------------------------------------------.\n";
    of << "| earth nutation:                                                    |\n";
    of << boost::format("| %=19s | %=14s %=14s %=14s |\n") %"time" %"X" %"Y" %"S";
    of << "|---------------------|----------------------------------------------|\n";
    for(int i=0; i<AstronomicalParameters::earth_nutTime.size(); ++i){
        of << boost::format("| %19s | %+14.6e %+14.6e %+14.6e |\n")
              % TimeSystem::time2string(AstronomicalParameters::earth_nutTime[i])
              % AstronomicalParameters::earth_nutX[i]
              % AstronomicalParameters::earth_nutY[i]
              % AstronomicalParameters::earth_nutS[i];
    }
    of << "'--------------------------------------------------------------------'\n\n";

}

void OperationNotes::displaySNRSummary(const Network &network,
                                       const std::vector<Source>& sources,
                                       const std::vector<Scan> & scans,
                                       const std::shared_ptr<const ObservingMode> &obsModes) {

    map<string,vector<vector<double>>> SNRs;
    const auto &bands = obsModes->getAllBands();

    for(const auto &band : bands){
        SNRs[band] = vector<vector<double>>(network.getNBls());
    }

    for(const auto &scan : scans){

        unsigned long srcid = scan.getSourceId();
        const auto &source = sources[srcid];

        for(int i=0; i<scan.getNObs(); ++i){
            const auto &obs = scan.getObservation(i);
            unsigned long blid = obs.getBlid();

            unsigned long staid1 = obs.getStaid1();
            unsigned long staid2 = obs.getStaid2();

            const auto &sta1 = network.getStation(staid1);
            const auto &sta2 = network.getStation(staid2);

            double el1 = scan.getPointingVector(static_cast<int>(*scan.findIdxOfStationId(staid1))).getEl();
            double el2 = scan.getPointingVector(static_cast<int>(*scan.findIdxOfStationId(staid2))).getEl();

            double date1 = 2400000.5;
            double date2 = TimeSystem::mjdStart + static_cast<double>(obs.getStartTime()) / 86400.0;
            double gmst = iauGmst82(date1, date2);

            unsigned int duration = obs.getObservingTime();

            for(const auto &band : bands){
                double observedFlux = source.observedFlux(band, gmst, network.getDxyz(staid1,staid2));

                double SEFD_sta1 = sta1.getEquip().getSEFD(band, el1);
                double SEFD_sta2 = sta2.getEquip().getSEFD(band, el2);

                double recordingRate = obsModes->getMode(0)->recordingRate(staid1, staid2, band);
                double efficiency = obsModes->getMode(0)->efficiency(sta1.getId(), sta2.getId());
                double snr = efficiency * observedFlux/(sqrt(SEFD_sta1 * SEFD_sta2)) * sqrt(recordingRate * duration);

                SNRs[band][blid].push_back(snr);
            }
        }
    }

    unsigned long nsta = network.getNSta();
    for(const auto &snr : SNRs){
        of << "average theoretical SNR for " << snr.first << "-band per baseline:\n";
        of << ".-----------";
        for (int i = 0; i < nsta-1; ++i) {
            of << "----------";
        }
        of << "-----------";
        of << "----------.\n";


        of << boost::format("| %8s |") % "STATIONS";
        for (const auto &any:network.getStations()) {
            of << boost::format(" %8s ") % any.getName();
        }
        of << "|";
        of << boost::format(" %8s ") % "AVERAGE";
        of << "|\n";

        of << "|----------|";
        for (int i = 0; i < nsta-1; ++i) {
            of << "----------";
        }
        of << "----------|";
        of << "----------|\n";

        vector<double> sumSNR(nsta,0.0);
        vector<int> counterSNR(nsta,0);
        for (unsigned long staid1 = 0; staid1 < nsta; ++staid1) {
            of << boost::format("| %8s |") % network.getStation(staid1).getName();
            for (unsigned long staid2 = 0; staid2 < nsta; ++staid2) {
                if (staid2<staid1+1){
                    of << "          ";
                }else{
                    unsigned long blid = network.getBaseline(staid1,staid2).getId();
                    if(snr.second[blid].empty()){
                        of << "        - ";
                    }else{
                        double SNR = accumulate(snr.second[blid].begin(), snr.second[blid].end(), 0.0);
                        int n = static_cast<int>(snr.second[blid].size());
                        sumSNR[staid1] += SNR;
                        counterSNR[staid1] +=n;
                        sumSNR[staid2] += SNR;
                        counterSNR[staid2] +=n;

                        of << boost::format(" %8.2f ") % (SNR/n);
                    }

                }
            }
            of << "|";
            of << boost::format(" %8.2f ") % (sumSNR[staid1]/counterSNR[staid1]);
            of << "|\n";
        }

        of << "'-----------";
        for (int i = 0; i < nsta-1; ++i) {
            of << "----------";
        }
        of << "-----------";
        of << "----------'\n\n";

    }

}

void OperationNotes::listKeys(const Network &network) {
    of << " Key:     ";
    int i = 0;
    for(const auto &station : network.getStations()){
        of << boost::format("%2s=%-8s   ") %station.getAlternativeName() %station.getName();
        ++i;
        if(i%5 == 0 && i<network.getNSta()){
            of << "\n          ";
        }
    }
    of << "\n\n";
}

void OperationNotes::displaySkyCoverageScore(const Network &network) {

    vector<double> a13m30;
    vector<double> a25m30;
    vector<double> a37m30;
    vector<double> a13m60;
    vector<double> a25m60;
    vector<double> a37m60;
    for (const auto &station : network.getStations()){
        unsigned long id = station.getId();
        const auto &map = network.getStaid2skyCoverageId();
        unsigned long skyCovId = map.at(id);
        const auto & skyCov = network.getSkyCoverage(skyCovId);
        a13m30.push_back(skyCov.getSkyCoverageScore_a13m30());
        a25m30.push_back(skyCov.getSkyCoverageScore_a25m30());
        a37m30.push_back(skyCov.getSkyCoverageScore_a37m30());
        a13m60.push_back(skyCov.getSkyCoverageScore_a13m60());
        a25m60.push_back(skyCov.getSkyCoverageScore_a25m60());
        a37m60.push_back(skyCov.getSkyCoverageScore_a37m60());
    }


    of << "sky coverage score (1 means perfect distribution)\n";
    of << ".--------------------";
    for (int i=0; i<network.getNSta(); ++i){
        of << boost::format("----------");
    }
    of << "-----------.\n";

    of << "|                   |";
    for (const auto &station : network.getStations()){
        of << boost::format(" %8s ") % station.getName();
    }
    of << "| average  |\n";

    of << "|-------------------|";
    for (int i=0; i<network.getNSta(); ++i){
        of << boost::format("----------");
    }
    of << "|----------|\n";

    of << "| 13 areas @ 30 min |";
    for ( double v : a13m30){
        of << boost::format(" %8.2f ") % v;
    }
    of << boost::format("| %8.2f |\n") % (accumulate(a13m30.begin(), a13m30.end(), 0.0)/network.getNSta());

    of << "| 25 areas @ 30 min |";
    for ( double v : a25m30){
        of << boost::format(" %8.2f ") % v;
    }
    of << boost::format("| %8.2f |\n") % (accumulate(a25m30.begin(), a25m30.end(), 0.0)/network.getNSta());

    of << "| 37 areas @ 30 min |";
    for ( double v : a37m30){
        of << boost::format(" %8.2f ") % v;
    }
    of << boost::format("| %8.2f |\n") % (accumulate(a37m30.begin(), a37m30.end(), 0.0)/network.getNSta());

    of << "|--------------------";
    for (const auto &station : network.getStations()){
        of << boost::format("----------") ;
    }
    of << "-----------|\n";

    of << "| 13 areas @ 60 min |";
    for ( double v : a13m60){
        of << boost::format(" %8.2f ") % v;
    }
    of << boost::format("| %8.2f |\n") % (accumulate(a13m60.begin(), a13m60.end(), 0.0)/network.getNSta());

    of << "| 25 areas @ 60 min |";
    for ( double v : a25m60){
        of << boost::format(" %8.2f ") % v;
    }
    of << boost::format("| %8.2f |\n") % (accumulate(a25m60.begin(), a25m60.end(), 0.0)/network.getNSta());

    of << "| 37 areas @ 60 min |";
    for ( double v : a37m60){
        of << boost::format(" %8.2f ") % v;
    }
    of << boost::format("| %8.2f |\n") % (accumulate(a37m60.begin(), a37m60.end(), 0.0)/network.getNSta());

    of << "'--------------------";
    for (int i=0; i<network.getNSta(); ++i){
        of << boost::format("----------");
    }
    of << "-----------'\n\n";

}



