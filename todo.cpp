//
//  main.cpp
//  Wizardry
//
//  Created by Timi Adewusi on 01/05/2025.
//

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <limits>
#include <fstream>

using namespace std;


const string FILE_NAME = "Tasks.txt";

void addTask(vector<string>& tasks);
void viewTask(vector<string>& tasks);
void markDone(vector<string>& tasks);
void deleteTask(vector<string>& tasks);
void loadTask(vector<string>& tasks);
void saveTask(const vector<string>& tasks);

void loadTask(vector<string>& tasks)
{
    ifstream file(FILE_NAME);
    string line;
    while(getline(file, line))
    {
        tasks.push_back(line);
    }
    file.close();
}

void saveTask(const vector<string>& tasks) {
    ofstream file(FILE_NAME);
    for (const string& task : tasks) {
        file << task << endl;
    }
    file.close();
}

void addTask(vector<string>& tasks)
{
    cout << "Enter a task: " << endl;
    
    string task;
    
    getline(cin, task);
    tasks.push_back(task);
    saveTask(tasks);
}

void viewTask(vector<string>& tasks)
{
    for(int i = 0; i < tasks.size(); i++)
    {
        cout << tasks[i] << endl;
    }
}

void markDone(vector<string>& tasks)
{
    for(int i = 0; i < tasks.size(); i++)
    {
        cout << tasks[i] << endl;
    }
    
    int input;
    cout << "Enter the task number to mark as done:" << endl;
    
    cin >> input;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    
    tasks[input - 1] += " .Done";
    saveTask(tasks);
    
}

void deleteTask(vector<string>& tasks)
{
    for(int i = 0; i < tasks.size(); i++)
    {
        cout << tasks[i] << endl;
    }
    
    int input;
    cout << "Enter the task number to delete:" << endl;
    
    cin >> input;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    
    tasks.erase(tasks.begin() + (input - 1));
    saveTask(tasks);
}
int main(int argc, const char * argv[])
{
    bool isRunning = true;
    
    vector<string> tasks;
    
    loadTask(tasks);
    
    while(isRunning)
    {
        char input;
        
        cout << "1. Add Task" << endl;
        cout << "2. View Task" << endl;
        cout << "3. Mark Task as Done" << endl;
        cout << "4. Delete Task" << endl;
        cout << "5. Exit" << endl;
        
        cin >> input;
        
        cin.ignore(numeric_limits<streamsize>::max(), '\n'); // flush leftover newline

        switch(input)
        {
            case '1':
                addTask(tasks);
                break;
            case '2':
                viewTask(tasks);
                break;
            case '3':
                markDone(tasks);
                break;
            case '4':
                deleteTask(tasks);
                break;
            default:
                isRunning = false;
                
        }
    }
    return 0;
}
