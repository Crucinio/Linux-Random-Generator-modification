#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <utility>
#include <cstdlib>
#include <limits>
#include <algorithm>
#include <cmath>

class GraphPlotter {
private:
    struct Graph {
        std::string name;
        std::vector<std::pair<double, double>> data;
        bool enabled = true;
        std::string lineStyle = "lines";  // Default style
        std::string color;                // Empty = auto
    };

    std::vector<Graph> graphs;
    std::string title = "Data Plot";
    std::string xLabel = "X Axis";
    std::string yLabel = "Y Axis";
    bool logX = false;
    bool logY = false;
    bool gridEnabled = true;
    bool showStats = true;

public:
    // Graph management
    void addGraph(const std::string& name, const std::vector<std::pair<double, double>>& points) {
        graphs.push_back({name, points, true});
    }

    void addGraph(const std::string& name, const std::vector<double>& discretes, double startX = 0, double step = 1) {
        std::vector<std::pair<double, double>> res;
        res.reserve(discretes.size());
        for (size_t i = 0; i < discretes.size(); ++i) {
            res.emplace_back(startX + step * i, discretes[i]);
        }
        graphs.push_back({name, res});
    }

    void removeGraph(size_t index) {
        if (index < graphs.size()) {
            graphs.erase(graphs.begin() + index);
        }
    }

    // Graph styling
    void setGraphStyle(size_t index, const std::string& style, const std::string& color = "") {
        if (index < graphs.size()) {
            graphs[index].lineStyle = style;
            graphs[index].color = color;
        }
    }

    // Plot configuration
    void setTitle(const std::string& newTitle) { title = newTitle; }
    void setXLabel(const std::string& label) { xLabel = label; }
    void setYLabel(const std::string& label) { yLabel = label; }
    void setLogScale(bool xLog, bool yLog) { logX = xLog; logY = yLog; }
    void setGrid(bool enabled) { gridEnabled = enabled; }
    void setShowStats(bool enabled) { showStats = enabled; }

    // Interactive controls
    void toggleGraphs() {
        while (true) {
            std::cout << "\nGraph Selection (" << graphs.size() << " graphs available):\n";
            for (size_t i = 0; i < graphs.size(); ++i) {
                std::cout << i << ": [" << (graphs[i].enabled ? "X" : " ") << "] "
                          << graphs[i].name << " (" << graphs[i].data.size() << " points)\n";
            }
            std::cout << "\nOptions:\n"
                      << "Enter index to toggle graph\n"
                      << "'s' to show statistics\n"
                      << "'c' to configure plot\n"
                      << "'q' to finish\n"
                      << "Your choice: ";

            std::string input;
            std::cin >> input;
            
            if (input == "q") break;
            if (input == "s") showGraphStatistics();
            else if (input == "c") configurePlot();
            else {
                try {
                    size_t index = std::stoul(input);
                    if (index < graphs.size()) {
                        graphs[index].enabled = !graphs[index].enabled;
                    } else {
                        std::cout << "Invalid index.\n";
                    }
                } catch (...) {
                    std::cout << "Invalid input.\n";
                }
            }
        }
    }

    // Plot generation
    void plot() {
        if (graphs.empty()) {
            std::cout << "No graphs to plot.\n";
            return;
        }

        std::vector<std::string> tempFiles;
        std::vector<std::string> plotCommands;

        // Generate data files
        for (size_t i = 0; i < graphs.size(); ++i) {
            if (!graphs[i].enabled) continue;

            std::string filename = "temp_graph_" + std::to_string(i) + ".dat";
            tempFiles.push_back(filename);


            std::ofstream outFile(filename);
            for (const auto& point : graphs[i].data) {
                outFile << point.first << " " << point.second << "\n";
            }
            outFile.close();

            // Build plot command for this graph
            std::string command = "'" + filename + "' title '" + graphs[i].name + "' with " + graphs[i].lineStyle;
            if (!graphs[i].color.empty()) {
                command += " linecolor rgb '" + graphs[i].color + "'";
            }
            plotCommands.push_back(command);
        }

        if (plotCommands.empty()) {
            std::cout << "No enabled graphs to plot.\n";
            return;
        }

        // Generate gnuplot script
        std::ofstream gp("plot_commands.gp");
        gp << "set terminal qt size 1000,700 enhanced font 'Verdana,12'\n";
        gp << "set title '" << title << "'\n";
        gp << "set xlabel '" << xLabel << "'\n";
        gp << "set ylabel '" << yLabel << "'\n";
        
        if (logX) gp << "set logscale x\n";
        if (logY) gp << "set logscale y\n";
        if (gridEnabled) gp << "set grid\n";
        
        gp << "set key outside right top\n";
        gp << "plot " << joinStrings(plotCommands, ", ") << "\n";
        
        if (showStats) {
            gp << "pause mouse close\n";  // Keep plot open after displaying stats
        }
        gp.close();

        // Execute gnuplot
        std::system("gnuplot -persist plot_commands.gp");

        // Cleanup temp files
        for (const auto& file : tempFiles) {
            std::remove(file.c_str());
        }
    }

private:
    // Helper functions
    std::string joinStrings(const std::vector<std::string>& strings, const std::string& delimiter) {
        std::string result;
        for (size_t i = 0; i < strings.size(); ++i) {
            if (i != 0) result += delimiter;
            result += strings[i];
        }
        return result;
    }

    void showGraphStatistics() {
        std::cout << "\nGraph Statistics:\n";
        for (size_t i = 0; i < graphs.size(); ++i) {
            if (!graphs[i].enabled) continue;
            
            const auto& data = graphs[i].data;
            if (data.empty()) continue;

            double minY = data[0].second, maxY = data[0].second;
            double sumY = 0;
            for (const auto& point : data) {
                minY = std::min(minY, point.second);
                maxY = std::max(maxY, point.second);
                sumY += point.second;
            }
            double avgY = sumY / data.size();

            std::cout << "Graph " << i << " (" << graphs[i].name << "):\n"
                      << "  Points: " << data.size() << "\n"
                      << "  X Range: [" << data.front().first << ", " << data.back().first << "]\n"
                      << "  Y Range: [" << minY << ", " << maxY << "]\n"
                      << "  Y Average: " << avgY << "\n\n";
        }
    }

    void configurePlot() {
        std::cout << "\nPlot Configuration:\n"
                  << "1. Title: " << title << "\n"
                  << "2. X Label: " << xLabel << "\n"
                  << "3. Y Label: " << yLabel << "\n"
                  << "4. Log Scale: X=" << (logX ? "on" : "off") 
                  << ", Y=" << (logY ? "on" : "off") << "\n"
                  << "5. Grid: " << (gridEnabled ? "on" : "off") << "\n"
                  << "6. Show Stats: " << (showStats ? "on" : "off") << "\n"
                  << "Select option to change (1-6) or 'q' to quit: ";

        std::string input;
        std::cin >> input;
        if (input == "q") return;


        try {
            int option = std::stoi(input);
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            
            switch (option) {
                case 1:
                    std::cout << "Enter new title: ";
                    std::getline(std::cin, title);
                    break;
                case 2:
                    std::cout << "Enter new X label: ";
                    std::getline(std::cin, xLabel);
                    break;
                case 3:
                    std::cout << "Enter new Y label: ";
                    std::getline(std::cin, yLabel);
                    break;
                case 4:
                    std::cout << "Toggle log scale for X axis? (y/n): ";
                    std::getline(std::cin, input);
                    if (input == "y") logX = !logX;
                    std::cout << "Toggle log scale for Y axis? (y/n): ";
                    std::getline(std::cin, input);
                    if (input == "y") logY = !logY;
                    break;
                case 5:
                    gridEnabled = !gridEnabled;
                    break;
                case 6:
                    showStats = !showStats;
                    break;
                default:
                    std::cout << "Invalid option.\n";
            }
        } catch (...) {
            std::cout << "Invalid input.\n";
        }
    }
};

