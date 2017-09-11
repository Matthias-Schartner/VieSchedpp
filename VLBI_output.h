/**
 * @file VLBI_output.h
 * @brief class VLBI_output
 *
 *
 * @author Matthias Schartner
 * @date 22.08.2017
 */

#ifndef VLBI_OUTPUT_H
#define VLBI_OUTPUT_H
#include "VLBI_scheduler.h"

namespace VieVS{
    class VLBI_output {
    public:
        /**
         * @brief empty default constructor
         */
        VLBI_output();

        /**
         * @brief constructor
         *
         * @param sched scheduler
         */
        VLBI_output(const VLBI_scheduler &sched);

        void setIsched(int isched) {
            VLBI_output::isched = isched;
        }

        /**
         * @brief displays some basic statistics of the schedule
         */
       void displayStatistics(bool general, bool station, bool source, bool baseline, bool duration);

        void writeNGS();

    private:
        int isched;
        vector<VLBI_station> stations; ///< all stations
        vector<VLBI_source> sources; ///< all sources
        vector<VLBI_skyCoverage> skyCoverages; ///< all sky coverages
        vector<VLBI_scan> scans; ///< all scans in schedule

        /**
         * @brief displays some general statistics of the schedule
         */
        void displayGeneralStatistics(ofstream &out);

        /**
         * @brief displays some baseline dependent statistics of the schedule
         */
        void displayBaselineStatistics(ofstream &out);

        /**
         * @brief displays some station dependent statistics of the schedule
         */
        void displayStationStatistics(ofstream &out);

        /**
         * @brief displays some source dependent statistics of the schedule
         */
        void displaySourceStatistics(ofstream &out);

        /**
         * @brief displays some source dependent statistics of the schedule
         */
        void displayScanDurationStatistics(ofstream &out);


    };
}


#endif //VLBI_OUTPUT_H
