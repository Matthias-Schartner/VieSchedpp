//
// Created by mschartn on 22.08.17.
//

#include "Output.h"
using namespace std;
using namespace VieVS;

Output::Output() = default;

Output::Output(const Scheduler &sched):
        stations_{sched.getStations()}, sources_{sched.getSources()}, skyCoverages_{sched.getSkyCoverages()},
        scans_{sched.getScans()}{

}

void Output::displayStatistics(bool general, bool station, bool source, bool baseline, bool duration) {

    string fname;
    if (iSched_ == 0) {
        fname = "statistics.txt";
    } else {
        fname = (boost::format("statistics_%04d.txt") % (iSched_)).str();
    }
    ofstream out(fname);

    string txt = (boost::format("writing statistics to %s\n") % fname).str();
    cout << txt;

    if (general) {
        displayGeneralStatistics(out);
    }

    if (station) {
        displayStationStatistics(out);
    }

    if (source) {
        displaySourceStatistics(out);
    }

    if (baseline) {
        displayBaselineStatistics(out);
    }

    if (duration) {
        displayScanDurationStatistics(out);
    }
    out.close();

}

void Output::displayGeneralStatistics(ofstream &out) {
    unsigned long n_scans = scans_.size();
    unsigned long n_single = 0;
    unsigned long n_subnetting = 0;
    unsigned long n_fillin = 0;

    for (const auto&any:scans_){
        Scan::ScanType thisType = any.getType();
        switch (thisType){
            case Scan::ScanType::single:{
                ++n_single;
                break;
            }
            case Scan::ScanType::subnetting:{
                ++n_subnetting;
                break;
            }
            case Scan::ScanType::fillin:{
                ++n_fillin;
                break;
            }
        }
    }

    out << "number of total scans:         " << n_scans << "\n";
    out << "number of single source scans: " << n_single << "\n";
    out << "number of subnetting scans:    " << n_subnetting << "\n";
    out << "number of fillin mode scans:   " << n_fillin << "\n\n";

}

void Output::displayBaselineStatistics(ofstream &out) {
    unsigned long nsta = stations_.size();
    unsigned long n_bl = 0;
    vector< vector <unsigned long> > bl_storage(nsta,vector<unsigned long>(nsta,0));
    for (const auto&any:scans_){
        unsigned long this_n_bl = any.getNBl();
        n_bl += this_n_bl;

        for (int i = 0; i < this_n_bl; ++i) {
            int staid1 = any.getBaseline(i).getStaid1();
            int staid2 = any.getBaseline(i).getStaid2();
            if(staid1>staid2){
                swap(staid1,staid2);
            }
            ++bl_storage[staid1][staid2];
        }

    }

    out << "number of scheduled baselines: " << n_bl << "\n";
    out << ".-----------";
    for (int i = 0; i < nsta-1; ++i) {
        out << "----------";
    }
    out << "-----------";
    out << "----------.\n";


    out << boost::format("| %8s |") % "STATIONS";
    for (int i = 0; i < nsta; ++i) {
        out << boost::format(" %8s ") % stations_[i].getName();
    }
    out << "|";
    out << boost::format(" %8s ") % "TOTAL";
    out << "|\n";

    out << "|----------|";
    for (int i = 0; i < nsta-1; ++i) {
        out << "----------";
    }
    out << "----------|";
    out << "----------|\n";

    for (int i = 0; i < nsta; ++i) {
        unsigned long counter = 0;
        out << boost::format("| %8s |") % stations_[i].getName();
        for (int j = 0; j < nsta; ++j) {
            if (j<i+1){
                out << "          ";
                counter += bl_storage[j][i];
            }else{
                out << boost::format(" %8d ") % bl_storage[i][j];
                counter += bl_storage[i][j];
            }
        }
        out << "|";
        out << boost::format(" %8d ") % counter;
        out << "|\n";
    }

    out << "'-----------";
    for (int i = 0; i < nsta-1; ++i) {
        out << "----------";
    }
    out << "-----------";
    out << "----------'\n\n";

}

void Output::displayStationStatistics(ofstream &out) {
    vector<unsigned int> nscan_sta(stations_.size());
    vector< vector<unsigned int> > time_sta(stations_.size());
    vector< unsigned int> nbl_sta(stations_.size(),0);

    for (auto& any:scans_){
        for (int ista = 0; ista < any.getNSta(); ++ista) {
            const PointingVector& pv =  any.getPointingVector(ista);
            int id = pv.getStaid();
            ++nscan_sta[id];
            time_sta[id].push_back(any.maxTime());
        }
        for (int ibl = 0; ibl < any.getNBl(); ++ibl){
            const Baseline &bl = any.getBaseline(ibl);
            ++nbl_sta[bl.getStaid1()];
            ++nbl_sta[bl.getStaid2()];
        }

    }

    out
            << ".------------------------------------------------------------------------------------------------------------------------.\n";
    out
            << "|          time since session start (1 char equals 15 minutes)                                                           |\n";
    out
            << "| STATION |0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15  16  17  18  19  20  21  22  23  | #SCANS #OBS |\n";
    out
            << "|---------|------------------------------------------------------------------------------------------------|-------------|\n";
    for (int i = 0; i<stations_.size(); ++i){
        out << boost::format("| %8s|") % stations_[i].getName();
        unsigned int timeStart = 0;
        unsigned int timeEnd = 900;
        for (int j = 0; j < 96; ++j) {
            if(any_of(time_sta[i].begin(), time_sta[i].end(), [timeEnd,timeStart](unsigned int k){return k<timeEnd && k>=timeStart;})){
                out << "x";
            }else{
                out << " ";
            }
            timeEnd += 900;
            timeStart += 900;
        }
        out << boost::format("| %6d %4d |\n") % nscan_sta[i] % nbl_sta[i];
    }

    out
            << "'------------------------------------------------------------------------------------------------------------------------'\n\n";

}

void Output::displaySourceStatistics(ofstream &out) {
    out << "number of available sources:   " << sources_.size() << "\n";
    vector<unsigned int> nscan_src(sources_.size(),0);
    vector<unsigned int> nbl_src(sources_.size(),0);
    vector< vector<unsigned int> > time_src(sources_.size());

    for (const auto& any:scans_){
        int id = any.getSourceId();
        ++nscan_src[id];
        nbl_src[id] += any.getNBl();
        time_src[id].push_back(any.maxTime());
    }
    long number = count_if(nscan_src.begin(), nscan_src.end(), [](int i) {return i > 0;});
    out << "number of scheduled sources:   " << number << "\n";
    out
            << ".------------------------------------------------------------------------------------------------------------------------.\n";
    out
            << "|          time since session start (1 char equals 15 minutes)                                                           |\n";
    out
            << "|  SOURCE |0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15  16  17  18  19  20  21  22  23  | #SCANS #OBS |\n";
    out
            << "|---------|------------------------------------------------------------------------------------------------|-------------|\n";
    for (int i = 0; i<sources_.size(); ++i){
        if (sources_[i].getNbls() == 0) {
            continue;
        }
        out << boost::format("| %8s|") % sources_[i].getName();
        unsigned int timeStart = 0;
        unsigned int timeEnd = 900;
        for (int j = 0; j < 96; ++j) {
            if(any_of(time_src[i].begin(), time_src[i].end(), [timeEnd,timeStart](unsigned int k){return k<timeEnd && k>=timeStart;})){
                out << "x";
            }else{
                out << " ";
            }
            timeEnd += 900;
            timeStart += 900;
        }
        out << boost::format("| %6d %4d |\n") % nscan_src[i] % nbl_src[i];
    }
    out
            << "'------------------------------------------------------------------------------------------------------------------------'\n\n";

}

void Output::displayScanDurationStatistics(ofstream &out) {
    unsigned long nsta = stations_.size();
    out << "required scan durations:\n";
    vector< vector< vector< unsigned int > > > bl_durations(nsta,vector<vector<unsigned int> >(nsta));
    vector< unsigned int> maxScanDurations;

    for(const auto&any: scans_){
        unsigned long nbl = any.getNBl();
        unsigned int endScan = any.maxTime();
        unsigned int startScan = numeric_limits<unsigned int>::max();
        for (int i = 0; i < any.getNSta(); ++i) {
            unsigned int thisStart = any.getTimes().getEndOfCalibrationTime(i);
            if (thisStart<startScan){
                startScan = thisStart;
            }
        }

        maxScanDurations.push_back(endScan-startScan);

        for (int i = 0; i < nbl; ++i) {
            const Baseline &bl = any.getBaseline(i);
            int staid1 = bl.getStaid1();
            int staid2 = bl.getStaid2();
            unsigned int bl_duration = bl.getScanDuration();

            if(staid1<staid2){
                swap(staid1,staid2);
            }
            bl_durations[staid1][staid2].push_back(bl_duration);
        }

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

    out << "scan duration (without corsynch):\n";
    for (int i = 0; i < hist.size(); ++i) {
        out << boost::format("%3d-%3d | ") % bins[i] % (bins[i + 1] - 1);
        double percent = 100*(double)hist[i]/double(maxScanDurations.size());
        percent = round(percent);
        for (int j = 0; j < percent; ++j) {
            out << "+";
        }
        out << "\n";
    }
    out << "\n";

    out << "scan length:\n";
    out << ".----------------------------------------------------------------.\n";
    out << "| STATION1-STATION2 |  min    10%    50%    90%    max   average |\n";
    out << "|----------------------------------------------------------------|\n";

    {
        int n = (int) maxScanDurations.size()-1;
        sort(maxScanDurations.begin(),maxScanDurations.end());
        out << boost::format("|       SCANS       | ");
        out << boost::format("%4d   ") % maxScanDurations[0];
        out << boost::format("%4d   ") % maxScanDurations[n * 0.1];
        out << boost::format("%4d   ") % maxScanDurations[n / 2];
        out << boost::format("%4d   ") % maxScanDurations[n * 0.9];
        out << boost::format("%4d   ") % maxScanDurations[n];
        double average = accumulate(maxScanDurations.begin(), maxScanDurations.end(), 0.0) / (n + 1);
        out << boost::format("%7.2f |") % average;
        out << "\n";
    }

    out << "|----------------------------------------------------------------|\n";

    for (int i = 1; i < nsta; ++i) {
        for (int j = 0; j < i; ++j) {
            vector<unsigned int>& this_duration = bl_durations[i][j];
            if(this_duration.empty()){
                continue;
            }
            int n = (int) this_duration.size()-1;
            sort(this_duration.begin(),this_duration.end());
            out << boost::format("| %8s-%8s | ") % stations_[i].getName() % stations_[j].getName();
            out << boost::format("%4d   ") % this_duration[0];
            out << boost::format("%4d   ") % this_duration[n * 0.1];
            out << boost::format("%4d   ") % this_duration[n / 2];
            out << boost::format("%4d   ") % this_duration[n * 0.9];
            out << boost::format("%4d   ") % this_duration[n];
            double average = accumulate(this_duration.begin(), this_duration.end(), 0.0) / (double) (n + 1);
            out << boost::format("%7.2f |") % average;
            out << "\n";
        }

    }
    out << "'----------------------------------------------------------------'\n\n";
}

void Output::writeNGS() {

    string fname;
    if (iSched_ == 0) {
        fname = "ngs.txt";
    } else {
        fname = (boost::format("ngs_%04d.txt") % (iSched_)).str();
    }
    ofstream out(fname);

    string txt = (boost::format("writing NGS file %s\n") % fname).str();
    cout << txt;

    boost::posix_time::ptime start = TimeSystem::startTime;
    unsigned long counter = 1;

    for (const auto &any: scans_) {
        for (int i = 0; i < any.getNBl(); ++i) {
            const Baseline &bl = any.getBaseline(i);
            string sta1 = stations_[bl.getStaid1()].getName();
            string sta2 = stations_[bl.getStaid2()].getName();
            if (sta1 > sta2) {
                swap(sta1, sta2);
            }
            string src = sources_[bl.getSrcid()].getName();
            unsigned int time = bl.getStartTime();

            boost::posix_time::ptime tmp = start + boost::posix_time::seconds(static_cast<long>(time));
            int year = tmp.date().year();
            int month = tmp.date().month();
            int day = tmp.date().day();
            int hour = tmp.time_of_day().hours();
            int minute = tmp.time_of_day().minutes();
            double second = tmp.time_of_day().seconds();

            out << boost::format("%8s  %8s  %8s %4d %02d %02d %02d %02d  %13.10f            ") % sta1 % sta2 % src %
                   year % month % day % hour % minute % second;
            out << boost::format("%6d") % counter << "01\n";

            out << "    0000000.00000000    .00000  -000000.0000000000    .00000 0      I   ";
            out << boost::format("%6d") % counter << "02\n";

            out << "    .00000    .00000    .00000    .00000   0.000000000000000        0.  ";
            out << boost::format("%6d") % counter << "03\n";

            out << "       .00   .0       .00   .0       .00   .0       .00   .0            ";
            out << boost::format("%6d") % counter << "04\n";

            out << "   -.00000   -.00000    .00000    .00000    .00000    .00000            ";
            out << boost::format("%6d") % counter << "05\n";

            out << "     0.000    00.000   000.000   000.000    00.000    00.000 0 0        ";
            out << boost::format("%6d") % counter << "06\n";

            out << "        0.0000000000    .00000        -.0000000000    .00000  0         ";
            out << boost::format("%6d") % counter << "08\n";

            out << "          0.00000000    .00000        0.0000000000    .00000 0      I   ";
            out << boost::format("%6d") % counter << "09\n";

            ++counter;
        }
    }

    out.close();
}

