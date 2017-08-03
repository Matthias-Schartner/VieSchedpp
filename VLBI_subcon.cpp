/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   VLBI_subcon.cpp
 * Author: mschartn
 * 
 * Created on June 29, 2017, 5:51 PM
 */

#include "VLBI_subcon.h"

bool VieVS::VLBI_subcon::subnetting = true;
bool VieVS::VLBI_subcon::fillinmode = true;


namespace VieVS{
    VLBI_subcon::VLBI_subcon(): n1scans{0}, n2scans{0} {
    }

    void VLBI_subcon::addScan(VLBI_scan scan){
        subnet1.push_back(scan);
        n1scans++;
    }

    void VLBI_subcon::calcStartTimes(vector<VLBI_station> &stations, vector<VLBI_source> &sources) {

        int i=0;
        while(i<n1scans){
            bool scanValid_slew = true;
            bool scanValid_idle = true;
            vector<unsigned  int> maxIdleTimes;

            int j=0;
            while(j<subnet1[i].getNSta()){
                int staid = subnet1[i].getStationId(j);

                stations[staid].unwrapAz(subnet1[i].getPointingVector(j));
                unsigned int slewtime = stations[staid].slewTime(subnet1[i].getPointingVector(j));

                if (slewtime > stations[staid].getMaxSlewtime()){
                    scanValid_slew = subnet1[i].removeElement(j);
                    if(!scanValid_slew){
                        break;
                    }
                } else {
                    maxIdleTimes.push_back(stations[staid].getMaxIdleTime());
                    subnet1[i].addTimes(j, stations[staid].getWaitSetup(), stations[staid].getWaitSource(), slewtime, stations[staid].getWaitTape(), stations[staid].getWaitCalibration());
                    ++j;
                }
            }

            if (scanValid_slew) {
                scanValid_idle = subnet1[i].checkIdleTimes(maxIdleTimes);
            }

            if (!scanValid_slew || !scanValid_idle){
                subnet1.erase(subnet1.begin()+i);
                --n1scans;
            }else{
                ++i;
            }
        }
    }
    
    void VLBI_subcon::constructAllBaselines(){
        for (auto& any: subnet1){
            any.constructBaselines();
        }
    }

    void VLBI_subcon::updateAzEl(vector<VLBI_station> &stations, vector<VLBI_source> &sources) {
        int i = 0;
        while (i < n1scans) {

            VLBI_source thisSource = sources[subnet1[i].getSourceId()];
            bool scanValid_slew = true;
            bool scanValid_idle = true;
            vector<unsigned  int> maxIdleTimes;

            int j = 0;
            while (j < subnet1[i].getNSta()) {
                int staid = subnet1[i].getPointingVector(j).getStaid();
                VLBI_pointingVector &thisPointingVector = subnet1[i].getPointingVector(j);
                thisPointingVector.setTime(subnet1[i].getTimes().getEndOfIdleTime(j));
                bool visible = stations[staid].isVisible(thisSource,thisPointingVector,false);
                unsigned int slewtime = numeric_limits<unsigned int>::max();

                if (visible){
                    stations[staid].unwrapAz(thisPointingVector);
                    slewtime = stations[staid].slewTime(thisPointingVector);
                }

                if (!visible || slewtime > stations[staid].getMaxSlewtime()){
                    scanValid_slew = subnet1[i].removeElement(j);
                    if(!scanValid_slew){
                        break;
                    }
                } else {
                    maxIdleTimes.push_back(stations[staid].getMaxIdleTime());
                    subnet1[i].updateSlewtime(j, slewtime);
                    ++j;
                }
            }


            if (scanValid_slew) {
                scanValid_idle = subnet1[i].checkIdleTimes(maxIdleTimes);
            }

            if (!scanValid_slew || !scanValid_idle){
                subnet1.erase(subnet1.begin()+i);
                --n1scans;
            }else{
                ++i;
            }
        }
    }

    void
    VLBI_subcon::calcAllBaselineDurations(vector<VLBI_station> &stations, vector<VLBI_source> &sources, double mjdStart) {
        for (int i = 0; i < n1scans; ++i) {
            VLBI_scan& thisScan = subnet1[i];
            thisScan.calcBaselineScanDuration(stations, sources[thisScan.getSourceId()], mjdStart);
        }
    }

    void VLBI_subcon::calcAllScanDurations(vector<VLBI_station>& stations, vector<VLBI_source>& sources) {
        int i=0;
        while (i<n1scans){
            VLBI_scan& thisScan = subnet1[i];
            int srcid = thisScan.getSourceId();

            bool scanValid = thisScan.scanDuration(stations, sources[srcid]);
            if (scanValid){
                ++i;
            } else {
                --n1scans;
                subnet1.erase(subnet1.begin()+i);
            }
        }
    }

    void VLBI_subcon::createSubcon2(vector<vector<int>> &subnettingSrcIds, int minStaPerSubcon) {
        vector<int> sourceIds(n1scans);
        for (int i = 0; i < n1scans; ++i) {
            sourceIds[i] = subnet1[i].getSourceId();
        }

        for (int i = 0; i < n1scans; ++i) {
            int firstSrcId = sourceIds[i];
            VLBI_scan &first = subnet1[i];
            vector<int> secondSrcIds = subnettingSrcIds[firstSrcId];
            for (int j = 0; j < secondSrcIds.size(); ++j) {
                vector<int> sta1 = first.getStationIds();

                int secondSrcId = secondSrcIds[j];
                auto it = find(sourceIds.begin(), sourceIds.end(), secondSrcId);
                if (it != sourceIds.end()) {
                    long idx = distance(sourceIds.begin(), it);
                    VLBI_scan &second = subnet1[idx];
                    vector<int> sta2 = second.getStationIds();


                    vector<int> uniqueSta1;
                    vector<int> uniqueSta2;
                    vector<int> intersection;
                    for (int any: sta1) {
                        if (find(sta2.begin(), sta2.end(), any) == sta2.end()) {
                            uniqueSta1.push_back(any);
                        } else {
                            intersection.push_back(any);
                        }
                    }
                    for (int any: sta2) {
                        if (find(sta1.begin(), sta1.end(), any) == sta1.end()) {
                            uniqueSta2.push_back(any);
                        }
                    }

                    if (uniqueSta1.size() + uniqueSta2.size() + intersection.size() < minStaPerSubcon) {
                        continue;
                    }

                    unsigned long nint = intersection.size();


                    for (int igroup = 0; igroup <= nint; ++igroup) {

                        vector<int> data(nint, 1);
                        for (unsigned long ii = nint - igroup; ii < nint; ++ii) {
                            data.at(ii) = 2;
                        }


                        do {
                            vector<int> scan1sta{uniqueSta1};
                            vector<int> scan2sta{uniqueSta2};
                            for (unsigned long ii = 0; ii < nint; ++ii) {
                                if (data.at(ii) == 1) {
                                    scan1sta.push_back(intersection[ii]);
                                } else {
                                    scan2sta.push_back(intersection[ii]);
                                }
                            }
                            if (scan1sta.size() >= first.getMinimumNumberOfStations() &&
                                scan2sta.size() >= second.getMinimumNumberOfStations()) {


                                bool firstValid;
                                VLBI_scan &&new_first = first.copyScan(scan1sta, firstValid);
                                if (!firstValid) {
                                    continue;
                                }

                                bool secondValid;
                                VLBI_scan &&new_second = second.copyScan(scan2sta, secondValid);
                                if (!secondValid) {
                                    continue;
                                }

                                ++n2scans;
                                pair<VLBI_scan, VLBI_scan> tmp;
                                tmp.first = move(new_first);
                                tmp.second = move(new_second);
                                subnet2.push_back(move(tmp));
                            }
                        } while (next_permutation(std::begin(data), std::end(data)));
                    }
                }
            }
        }
    }


    void VLBI_subcon::generateScore(vector<VLBI_station> &stations,
                                    vector<VLBI_skyCoverage> &skyCoverages) {
        unsigned long nmaxsta = stations.size();
        for (auto &thisScan: subnet1) {
            thisScan.calcScore(nmaxsta, nmaxbl, astas, asrcs, minTime, maxTime, skyCoverages);
        }

        for (auto &thisScans:subnet2) {
            VLBI_scan &thisScan1 = thisScans.first;
            thisScan1.calcScore(nmaxsta, nmaxbl, astas, asrcs, minTime, maxTime, skyCoverages);

            VLBI_scan &thisScan2 = thisScans.second;
            thisScan2.calcScore(nmaxsta, nmaxbl, astas, asrcs, minTime, maxTime, skyCoverages);
        }
    }

    void VLBI_subcon::minMaxTime() {
        unsigned int maxTime_ = 0;
        unsigned int minTime_ = numeric_limits<unsigned int>::max();
        for (auto &thisScan: subnet1) {
            unsigned int thisTime = thisScan.maxTime();
            if (thisTime < minTime_) {
                minTime_ = thisTime;
            }
            if (thisTime > maxTime_) {
                maxTime_ = thisTime;
            }
        }
        for (auto &thisScan: subnet2) {
            unsigned int thisTime1 = thisScan.first.maxTime();
            if (thisTime1 < minTime_) {
                minTime_ = thisTime1;
            }
            if (thisTime1 > maxTime_) {
                maxTime_ = thisTime1;
            }
            unsigned int thisTime2 = thisScan.first.maxTime();
            if (thisTime2 < minTime_) {
                minTime_ = thisTime2;
            }
            if (thisTime2 > maxTime_) {
                maxTime_ = thisTime2;
            }
        }
        minTime = minTime_;
        maxTime = maxTime_;
    }


    void VLBI_subcon::average_station_score(const vector<VLBI_station> &stations) {

        vector<double> staobs;
        for (auto &thisStation:stations) {
            staobs.push_back(thisStation.getNbls());
        }

        double meanStaobs = std::accumulate(std::begin(staobs), std::end(staobs), 0.0) / staobs.size();

        double total = 0;
        vector<double> staobs_score;
        for (auto &thisStaobs:staobs) {
            double diff = meanStaobs - thisStaobs;
            if (diff > 0) {
                staobs_score.push_back(diff);
                total += diff;
            } else {
                staobs_score.push_back(0);
            }
        }
        if (total != 0) {
            for (auto &thisStaobs_score:staobs_score) {
                thisStaobs_score /= total;
            }
        }
        astas = staobs_score;
    }

    void VLBI_subcon::average_source_score(vector<VLBI_source> &sources) {
        vector<double> srcobs;
        for (auto &thisSource:sources) {
            srcobs.push_back(thisSource.getNbls());
        }
        double meanSrcobs = std::accumulate(std::begin(srcobs), std::end(srcobs), 0.0) / srcobs.size();

        double max = 0;
        vector<double> srcobs_score;
        for (auto &thisSrcobs:srcobs) {
            double diff = meanSrcobs - thisSrcobs;
            if (diff > 0) {
                srcobs_score.push_back(diff);
                if (diff > max) {
                    max = diff;
                }
            } else {
                srcobs_score.push_back(0);
            }
        }
        if (max != 0) {
            for (auto &thisStaobs_score:srcobs_score) {
                thisStaobs_score /= max;
            }
        }
        asrcs = srcobs_score;
    }

    void VLBI_subcon::calcScores() {
        for (auto &thisScan: subnet1) {
            subnet1_score.push_back(thisScan.getScore());
        }
        for (auto &thisScans: subnet2) {
            VLBI_scan &thisScan1 = thisScans.first;
            double score1 = thisScan1.getScore();
            VLBI_scan &thisScan2 = thisScans.second;
            double score2 = thisScan2.getScore();
            subnet2_score.push_back(score1 + score2);
        }
    }

    void VLBI_subcon::precalcScore(vector<VLBI_station> &stations, vector<VLBI_source> &sources) {

        unsigned long nsta = stations.size();
        nmaxbl = (nsta * (nsta - 1)) / 2;
        minMaxTime();
        average_station_score(stations);
        average_source_score(sources);
    }

    unsigned long VLBI_subcon::rigorousScore(vector<VLBI_station> &stations, vector<VLBI_source> &sources,
                                             vector<VLBI_skyCoverage> &skyCoverages, double mjdStart) {

        vector<double> scores = subnet1_score;
        scores.insert(scores.end(), subnet2_score.begin(), subnet2_score.end());

        std::priority_queue<std::pair<double, int>> q;
        for (int i = 0; i < scores.size(); ++i) {
            q.push(std::pair<double, int>(scores[i], i));
        }

        while (true) {
            int idx = q.top().second;
            q.pop();

            if (idx < n1scans) {
                int thisIdx = idx;
                VLBI_scan &thisScan = subnet1[thisIdx];

                bool flag = thisScan.rigorousUpdate(stations, sources[thisScan.getSourceId()], mjdStart);
                thisScan.calcScore(stations.size(), nmaxbl, astas, asrcs, minTime, maxTime, skyCoverages);
                double newScore = thisScan.getScore();
                if (!flag) {
                    continue;
                }

                q.push(make_pair(newScore, idx));

            } else {
                int thisIdx = idx - n1scans;
                auto &thisScans = subnet2[thisIdx];

                VLBI_scan &thisScan1 = thisScans.first;
                bool flag1 = thisScan1.rigorousUpdate(stations, sources[thisScan1.getSourceId()], mjdStart);
                thisScan1.calcScore(stations.size(), nmaxbl, astas, asrcs, minTime, maxTime, skyCoverages);
                double newScore1 = thisScan1.getScore();
                if (!flag1) {
                    continue;
                }

                VLBI_scan &thisScan2 = thisScans.second;
                bool flag2 = thisScan2.rigorousUpdate(stations, sources[thisScan2.getSourceId()], mjdStart);
                thisScan2.calcScore(stations.size(), nmaxbl, astas, asrcs, minTime, maxTime, skyCoverages);
                double newScore2 = thisScan2.getScore();
                if (!flag2) {
                    continue;
                }
                double newScore = newScore1 + newScore2;

                q.push(make_pair(newScore, idx));
            }
            int newIdx = q.top().second;
            if (newIdx == idx) {
                return idx;
            }
        }
    }


}
