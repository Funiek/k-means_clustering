// #include <omp.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <vector>
#include <chrono>
#define tp std::chrono::time_point<std::chrono::high_resolution_clock>

using namespace std;

class Point
{
private:
    int pointId, clusterId;
    int dimensions;
    vector<double> values;

    vector<double> lineToVec(string &line)
    {
        vector<double> values;
        string tmp = "";

        for (int i = 0; i < (int)line.length(); i++)
        {
            if ((48 <= int(line[i]) && int(line[i])  <= 57) || line[i] == '.' || line[i] == '+' || line[i] == '-' || line[i] == 'e')
            {
                tmp += line[i];
            }
            else if (tmp.length() > 0)
            {

                values.push_back(stod(tmp));
                tmp = "";
            }
        }
        if (tmp.length() > 0)
        {
            values.push_back(stod(tmp));
            tmp = "";
        }

        return values;
    }

public:
    Point(int id, string line)
    {
        pointId = id;
        values = lineToVec(line);
        dimensions = values.size();
        clusterId = 0; // Initially not assigned to any cluster
    }

    int getDimensions() { return dimensions; }

    int getCluster() { return clusterId; }

    int getID() { return pointId; }

    void setCluster(int val) { clusterId = val; }

    double getVal(int pos) { return values[pos]; }
};

class Cluster
{
private:
    int clusterId;
    vector<double> centroid;
    vector<Point> points;

public:
    Cluster(int clusterId, Point centroid)
    {
        this->clusterId = clusterId;
        for (int i = 0; i < centroid.getDimensions(); i++)
        {
            this->centroid.push_back(centroid.getVal(i));
        }
        this->addPoint(centroid);
    }

    void addPoint(Point p)
    {
        p.setCluster(this->clusterId);
        points.push_back(p);
    }

    bool removePoint(int pointId)
    {
        int size = points.size();

        for (int i = 0; i < size; i++)
        {
            if (points[i].getID() == pointId)
            {
                points.erase(points.begin() + i);
                return true;
            }
        }
        return false;
    }

    void removeAllPoints() { points.clear(); }

    int getId() { return clusterId; }

    Point getPoint(int pos) { return points[pos]; }

    int getSize() { return points.size(); }

    double getCentroidByPos(int pos) { return centroid[pos]; }

    void setCentroidByPos(int pos, double val) { this->centroid[pos] = val; }
};

class KMeans
{
private:
    int K, iters, dimensions, total_points;
    vector<Cluster> clusters;
    string output_prefix;
    string line;
    tp start;
    tp finish;

    void clearClusters()
    {
        for (int i = 0; i < K; i++)
        {
            clusters[i].removeAllPoints();
        }
    }

    int getNearestClusterId(Point point)
    {
        double sum = 0.0, min_dist;
        int NearestClusterId;
        if(dimensions==1) {
            min_dist = abs(clusters[0].getCentroidByPos(0) - point.getVal(0));
        }	
        else 
        {
          for (int i = 0; i < dimensions; i++)
          {
             sum += pow(clusters[0].getCentroidByPos(i) - point.getVal(i), 2.0);
             // sum += abs(clusters[0].getCentroidByPos(i) - point.getVal(i));
          }
          min_dist = sqrt(sum);
        }
        NearestClusterId = clusters[0].getId();

        for (int i = 1; i < K; i++)
        {
            double dist;
            sum = 0.0;
            
            if(dimensions==1) {
                  dist = abs(clusters[i].getCentroidByPos(0) - point.getVal(0));
            } 
            else {
              for (int j = 0; j < dimensions; j++)
              {
                  sum += pow(clusters[i].getCentroidByPos(j) - point.getVal(j), 2.0);
                  // sum += abs(clusters[i].getCentroidByPos(j) - point.getVal(j));
              }

              dist = sqrt(sum);
              // dist = sum;
            }
            if (dist < min_dist)
            {
                min_dist = dist;
                NearestClusterId = clusters[i].getId();
            }
        }

        return NearestClusterId;
    }

public:
    KMeans(int K, int iterations, string output_prefix, string line)
    {
        this->K = K;
        this->iters = iterations;
        this->output_prefix = output_prefix;
        this->line = line;
    }

    void run(vector<Point> &all_points)
    {
        this->start = std::chrono::high_resolution_clock::now();
        total_points = all_points.size();
        dimensions = all_points[0].getDimensions();

        // Initializing Clusters
        vector<int> used_pointIds;

        for (int i = 1; i <= K; i++)
        {
            while (true)
            {
                int index = rand() % total_points;

                if (find(used_pointIds.begin(), used_pointIds.end(), index) ==
                    used_pointIds.end())
                {
                    used_pointIds.push_back(index);
                    all_points[index].setCluster(i);
                    Cluster cluster(i, all_points[index]);
                    clusters.push_back(cluster);
                    break;
                }
            }
        }
        // cout << "Clusters initialized = " << clusters.size() << endl
        //      << endl;

        // cout << "Running K-Means Clustering.." << endl;

        int iter = 1;
        while (true)
        {
            // cout << "Iter - " << iter << "/" << iters << endl;
            bool done = true;

            // Add all points to their nearest cluster
            // #pragma omp parallel for reduction(&&: done) num_threads(16)
            for (int i = 0; i < total_points; i++)
            {
                int currentClusterId = all_points[i].getCluster();
                int nearestClusterId = getNearestClusterId(all_points[i]);

                if (currentClusterId != nearestClusterId)
                {
                    all_points[i].setCluster(nearestClusterId);
                    done = false;
                }
            }

            // clear all existing clusters
            clearClusters();

            // reassign points to their new clusters
            for (int i = 0; i < total_points; i++)
            {
                // cluster index is ID-1
                clusters[all_points[i].getCluster() - 1].addPoint(all_points[i]);
            }

            // Recalculating the center of each cluster
            for (int i = 0; i < K; i++)
            {
                int ClusterSize = clusters[i].getSize();

                for (int j = 0; j < dimensions; j++)
                {
                    double sum = 0.0;
                    if (ClusterSize > 0)
                    {
                        // #pragma omp parallel for reduction(+: sum) num_threads(16)
                        for (int p = 0; p < ClusterSize; p++)
                        {
                            sum += clusters[i].getPoint(p).getVal(j);
                        }
                        clusters[i].setCentroidByPos(j, sum / ClusterSize);
                    }
                }
            }

            if (done || iter >= iters)
            {
                // cout << "Clustering completed in iteration : " << iter << endl
                //      << endl;
                break;
            }
            iter++;
        }

        this->finish = std::chrono::high_resolution_clock::now();

        // TODO zrobić z tym coś żeby zapis wykluczyć z wyników czasowych
        ofstream pointsFile;
        pointsFile.open(output_prefix + "_labels.txt", ios::out);

        // cout << "num of points: " << total_points << endl;
        pointsFile << total_points << endl;

        for (int i = 0; i < total_points; i++)
        {
            pointsFile << all_points[i].getCluster() << endl;
        }

        pointsFile.close();

        // Write cluster centers to file
        ofstream outfile;
        outfile.open(output_prefix + "_centroids.txt");
        if (outfile.is_open())
        {
            outfile << K << " " << dimensions << endl;
            for (int i = 0; i < K; i++)
            {
                // cout << "Cluster " << clusters[i].getId() << " centroid : ";
                for (int j = 0; j < dimensions; j++)
                {
                    // cout << clusters[i].getCentroidByPos(j) << " ";    // Output to console
                    outfile << clusters[i].getCentroidByPos(j) << " "; // Output to file
                }
                // cout << endl;
                outfile << endl;
            }
            outfile.close();
        }
        else
        {
            cout << "Error: Unable to write to clusters.txt";
        }
    }

    std::chrono::duration<double> get_execution_time()
    {
        return (this->finish - this->start);
    }
};

int main(int argc, char **argv)
{
    if (argc < 4 && argc > 5)
    {
        cout << "Error: command-line argument count mismatch. \n ./kmeans <INPUT> <K> <OUT-DIR> <SEED(OPTIONAL)>" << endl;
        return 1;
    }

    unsigned int seed = (argc == 5) ? (unsigned)atoi(argv[4]) : (unsigned)time(NULL);
    srand(seed);

    string output_prefix = argv[3];

    // Fetching number of clusters
    int K = atoi(argv[2]);

    // Open file for fetching points
    string filename = argv[1];
    ifstream infile(filename.c_str());

    if (!infile.is_open())
    {
        cout << "Error: Failed to open file." << endl;
        return 1;
    }

    // Fetching points from file
    int pointId = 1;
    vector<Point> all_points;
    string line;
    getline(infile, line);
    cout << "pierwsza linia " << line << endl;

    while (getline(infile, line))
    {
        Point point(pointId, line);
        all_points.push_back(point);
        pointId++;
    }
    
    infile.close();
    cout << "\nData fetched successfully!\n\n";

    // Return if number of clusters > number of points
    if ((int)all_points.size() < K)
    {
        cout << "Error: Number of clusters greater than number of points." << endl;
        return 1;
    }

    // Running K-Means Clustering
    int iters = 100000;
    
    KMeans kmeans(K, iters, output_prefix, line);
    kmeans.run(all_points);

    std::chrono::duration<double> elapsed = kmeans.get_execution_time();
    std::cout << "Czas wykonania: " << elapsed.count() << " sekundy." << std::endl;

    std::ofstream file(output_prefix + "_execution_time.txt", std::ios::app);
    if (file.is_open()) {
        file << elapsed.count() << endl;
        file.close();
    } else {
        std::cerr << "Nie udało się otworzyć pliku 'execution_time.txt'." << std::endl;
    }

    return 0;
}
