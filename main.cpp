#include <iostream>
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/statement.h>
#include <cppconn/resultset.h>
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>

using namespace std;

class Project
{
private:
    string name, id;
    int number_of_questions = 5;
    int num_of_participants;
    static const int MAX_PARTICIPANTS = 100; 

    sql::mysql::MySQL_Driver *driver;
    sql::Connection *con;

public:
    Project()
    {
        driver = dynamic_cast<sql::mysql::MySQL_Driver *>(get_driver_instance());
        con = driver->connect("localhost", "halid", "some_pass");
        con->setSchema("cppProject");
    }

    ~Project()
    {
        delete con;
    }

    void registerParticipants()
    {
        int n;
        cout << "\n--------------------------------------------Add Student Details----------------------------------------------" << endl
             << endl;
        cout << "\t\t\tEnter the number of participants: ";
        cin >> n;
        cin.ignore();
        num_of_participants = n;

        try
        {
            sql::PreparedStatement *stmt;
            stmt = con->prepareStatement("INSERT INTO Participants (participant_id, name) VALUES (?, ?)");

            for (int i = 0; i < n; i++)
            {
                cout << "\t\t\tEnter Participant " << i + 1 << " Name: ";
                getline(cin, name);
                cout << "\t\t\tEnter Participant " << i + 1 << " ID code: ";
                getline(cin, id);

                stmt->setString(1, id);
                stmt->setString(2, name);
                stmt->executeUpdate();
                cout << "\n\t\t\tParticipant " << i + 1 << " added successfully." << endl
                     << endl;
            }

            delete stmt;
        }
        catch (sql::SQLException &e)
        {
            cout << "\t\t\tError: " << e.what() << endl;
        }
    }

    void askQuestions()
    {
        int num_participants;
        cout << "Enter the number of participants taking the quiz: ";
        cin >> num_participants;

        for (int i = 0; i < num_participants; ++i)
        {
            cout << "\nParticipant " << i + 1 << " - Enter your ID: ";
            string participant_id;
            cin >> participant_id;

            
            bool participant_found = false;
            try
            {
                sql::PreparedStatement *stmt;
                stmt = con->prepareStatement("SELECT COUNT(*) AS count FROM Participants WHERE participant_id = ?");
                stmt->setString(1, participant_id);
                sql::ResultSet *res = stmt->executeQuery();

                if (res->next())
                {
                    int count = res->getInt("count");
                    if (count > 0)
                    {
                        participant_found = true;
                        cout << "Participant found. Starting quiz...\n";
                    }
                    else
                    {
                        cout << "Participant ID not found. Please enter a valid ID.\n";
                        --i; 
                    }
                }

                delete res;
                delete stmt;
            }
            catch (sql::SQLException &e)
            {
                cout << "\t\t\tError: " << e.what() << endl;
                return;
            }

            if (!participant_found)
                continue;

            
            try
            {
                sql::Statement *stmt;
                sql::PreparedStatement *prep_stmt;

                stmt = con->createStatement();
                sql::ResultSet *res = stmt->executeQuery("SELECT participant_id, name FROM Participants WHERE participant_id = '" + participant_id + "'");

                if (res->next())
                {
                    string participant_name = res->getString("name");
                    cout << "\nHi " << participant_name << "! Here are your questions:\n\n";

                    sql::Statement *q_stmt;
                    sql::ResultSet *q_res;
                    
                    q_stmt = con->createStatement();
                    q_res = q_stmt->executeQuery("SELECT question_id, question_text, option_a, option_b, option_c, option_d, correct_option FROM Questions ORDER BY RAND() LIMIT " + to_string(number_of_questions));

                    while (q_res->next())
                    {
                        int question_id = q_res->getInt("question_id");
                        string question_text = q_res->getString("question_text");
                        string option1 = q_res->getString("option_a");
                        string option2 = q_res->getString("option_b");
                        string option3 = q_res->getString("option_c");
                        string option4 = q_res->getString("option_d");
                        string correct_option = q_res->getString("correct_option");

                        cout << question_text << endl;
                        cout << "a. " << option1 << endl;
                        cout << "b. " << option2 << endl;
                        cout << "c. " << option3 << endl;
                        cout << "d. " << option4 << endl;

                        char selected_option;
                        cout << "Select your option (a-d): ";
                        cin >> selected_option;

                        bool is_correct = (string(1, selected_option) == correct_option);
                        cout << endl;
                        prep_stmt = con->prepareStatement("INSERT INTO Responses (participant_id, question_id, selected_option, is_correct) VALUES (?, ?, ?, ?)");
                        prep_stmt->setString(1, participant_id);
                        prep_stmt->setInt(2, question_id);
                        prep_stmt->setString(3, string(1, selected_option));
                        prep_stmt->setBoolean(4, is_correct);
                        prep_stmt->executeUpdate();

                        delete prep_stmt;
                    }

                    delete q_res;
                    delete q_stmt;
                }

                delete res;
                delete stmt;
            }
            catch (sql::SQLException &e)
            {
                cout << "\t\t\tError: " << e.what() << endl;
                return;
            }
        }
    }

    void displayLeaderboard()
    {
        try
        {
            sql::Statement *stmt;
            sql::PreparedStatement *prep_stmt;

            stmt = con->createStatement();
            sql::ResultSet *res = stmt->executeQuery("SELECT participant_id, COUNT(is_correct) as score FROM Responses WHERE is_correct = 1 GROUP BY participant_id ORDER BY score DESC");

            cout << "\n--------------------------------------------Leaderboard----------------------------------------------" << endl;
            int highest_score = 0;
            string winner_name;
            while (res->next())
            {
                string participant_id = res->getString("participant_id");
                int score = res->getInt("score");

                prep_stmt = con->prepareStatement("SELECT name FROM Participants WHERE participant_id = ?");
                prep_stmt->setString(1, participant_id);
                sql::ResultSet *name_res = prep_stmt->executeQuery();
                if (name_res->next())
                {
                    string name = name_res->getString("name");
                    cout << "Participant: " << name << " - Score: " << score << endl;

                    if (score > highest_score)
                    {
                        highest_score = score;
                        winner_name = name;
                    }
                }

                delete name_res;
                delete prep_stmt;
            }

            cout << "\nCongratulations to the winner: " << winner_name << "!" << endl;

            delete res;
            delete stmt;
        }
        catch (sql::SQLException &e)
        {
            cout << "\t\t\tError: " << e.what() << endl;
        }
    }

    void clearParticipantsData()
    {
        try
        {
            sql::Statement *stmt;

            stmt = con->createStatement();
            stmt->executeUpdate("DELETE FROM Participants");
            delete stmt;

            cout << "\t\t\tData from Participants table has been cleared." << endl;
        }
        catch (sql::SQLException &e)
        {
            cout << "\t\t\tError: " << e.what() << endl;
        }
    }
};

int main()
{
    int choice;
    Project quiz;

    do
    {
        cout << "\n\n\t\t\t ---------------------------" << endl;
        cout << "\t\t\t| Quiz Competition Manager |" << endl;
        cout << "\t\t\t ---------------------------" << endl;
        cout << "\t\t\t 1. Register Participants\n";
        cout << "\t\t\t 2. Start Quiz\n";
        cout << "\t\t\t 3. Display Leaderboard\n";
        cout << "\t\t\t 4. Clear Participants Data\n";
        cout << "\t\t\t 5. Exit\n";

        cout << "Enter your choice: ";
        cin >> choice;

        cout << "\t\t\t ---------------------------" << endl;
        cout << "\t\t\t Choose Option:[1/2/3/4/5]" << endl;
        cout << "\t\t\t ---------------------------" << endl;

        switch (choice)
        {
        case 1:
            quiz.registerParticipants();
            break;
        case 2:
            quiz.askQuestions();
            break;
        case 3:
            quiz.displayLeaderboard();
            break;
        case 4:
            quiz.clearParticipantsData();
            break;
        case 5:
            cout << "Exiting...\n";
            break;
        default:
            cout << "Invalid choice. Please try again.\n";
            break;
        }
    } while (choice != 5);

    return 0;
}
