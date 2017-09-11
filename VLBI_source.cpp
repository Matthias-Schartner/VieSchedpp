/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   VLBI_source.cpp
 * Author: mschartn
 * 
 * Created on June 28, 2017, 11:25 AM
 */

#include "VLBI_source.h"
namespace VieVS{
    VLBI_source::VLBI_source() {
    }

    VLBI_source::VLBI_source(const string &src_name, double src_ra_deg, double src_de_deg,
                             const vector<pair<string, VLBI_flux> > &src_flux) :
            name{src_name}, id{0}, ra{src_ra_deg * deg2rad}, de{src_de_deg * deg2rad}, flux{src_flux}, lastScan{0},
            nscans{0}, nbls{0} {

        PRECALC.sourceInCrs.resize(3);
        PRECALC.sourceInCrs[0] = cos(de)*cos(ra);
        PRECALC.sourceInCrs[1] = cos(de)*sin(ra);
        PRECALC.sourceInCrs[2] = sin(de);
    }
    
    VLBI_source::~VLBI_source() {
    }

    double VLBI_source::angleDistance(const VLBI_source &other) const noexcept {
        return acos(cos(de)*cos(other.de) * cos(ra-other.ra) + sin(de)*sin(other.de));
    }

    bool VLBI_source::isStrongEnough(double &maxFlux) const noexcept {
        maxFlux = 0;
        
        for (auto& any: flux){
            double thisFlux = any.second.getMaximumFlux();
            if (thisFlux > maxFlux){
                maxFlux = thisFlux;
            }
        }
        return maxFlux > *PARA.minFlux;
    }

    ostream &operator<<(ostream &out, const VLBI_source &src) noexcept {
        cout << boost::format("%=36s\n") % src.name;
        double ra_deg = src.ra*rad2deg;
        double de_deg = src.de*rad2deg;
        cout << "position:\n";
        cout << boost::format("  RA: %10.6f [deg]\n") % ra_deg;
        cout << boost::format("  DE: %10.6f [deg]\n") % de_deg;
//        cout << src.flux;
        cout << "------------------------------------\n";
        return out;
    }

    vector<pair<string, double> >
    VLBI_source::observedFlux(double gmst, double dx, double dy, double dz) const noexcept {
        vector<pair<string, double> > fluxes;

        double ha = gmst - ra;

        double u = dx * sin(ha) + dy * cos(ha);
        double v = dz*cos(de) + sin(de) * (-dx * cos(ha) + dy * sin(ha));

        for(auto& thisFlux: flux){
            double observedFlux = thisFlux.second.getFlux(u,v);

            fluxes.push_back(make_pair(thisFlux.first, observedFlux));
        }

        return std::move(fluxes);
    }

    void VLBI_source::update(unsigned long nbl, unsigned int time) noexcept {
        ++nscans;
        nbls += nbl;
        lastScan = time;
    }

    bool VLBI_source::checkForNewEvent(unsigned int time, bool &hardBreak, bool output, ofstream &bodyLog) noexcept {
        bool flag = false;
        while (EVENTS[nextEvent].time <= time && time != VieVS_time::duration) {
            double oldMinFlux = *PARA.minFlux;
            PARA = EVENTS[nextEvent].PARA;
            double newMinFlux = *PARA.minFlux;
            hardBreak = hardBreak || !EVENTS[nextEvent].softTransition;

            if (output) {
                bodyLog << "###############################################\n";
                bodyLog << "## changing parameters for source: " << boost::format("%8s") % name << "  ##\n";
                bodyLog << "###############################################\n";
            }

            nextEvent++;
            if (oldMinFlux != newMinFlux) {
                double maxFlux = 0;
                bool strongEnough = isStrongEnough(maxFlux);
                if (!strongEnough) {
                    setAvailable(false);
                    bodyLog << "source: " << boost::format("%8s") % name << " not strong enough! (max flux = "
                            << boost::format("%4.2f") % maxFlux << " min required flux = "
                            << boost::format("%4.2f") % *PARA.minFlux << ")\n";;
                }
                flag = true;
            }
        }
        return flag;
    }

}
