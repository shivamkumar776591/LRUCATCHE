#include <bits/stdc++.h>
using namespace std;

class LRUCache {
public:

    struct Node{
        Node* next;
        Node* prev;
        int val;
        int key;
        public:
        Node(int data,int key){
            next = NULL;
            prev = NULL;
            val = data;
            this->key = key;
        }
    };
        Node* head= new Node(-1,-1);
        Node* tail = new Node(-1,-1);
       
        unordered_map<int,Node*>mp;
        void insertAtStart(int key,int data){
            Node* newNode = new Node(data,key);
            mp[key] = newNode;
            if(head->next == tail){
                head->next = newNode;
                newNode->next = tail;
                newNode->prev = head;
                tail->prev = newNode;
             
                return;
            }
            newNode->next = head->next;
            newNode->next->prev = newNode;
            head->next = newNode;
            newNode->prev = head;
            

        }
        void deleteNode(int key){
            Node* node = mp[key];
            Node* prevNode = node->prev;
           
            prevNode->next = node->next;
            node->next->prev = prevNode;
           node->prev = NULL;
           node->next = NULL;
           delete(node);
           mp.erase(key);
        }
        void deleteFromTail(){
            Node* node = tail->prev;
          
             Node* prevNode = node->prev;
               prevNode->next = tail;
               tail->prev  = prevNode;
             mp.erase(node->key);
           
           delete(node);
          
        }
  int cap;
    
    
    LRUCache(int capacity) {
        cap = capacity;
         head->next = tail;
        tail->prev = head;
    }
    
    int get(int key) {
        if(mp.count(key)){
            int data = mp[key]->val;
            deleteNode(key);
            insertAtStart(key,data);
            return mp[key]->val;
        }
        return -1;
    }
    
    void put(int key, int value) {
      
       if(mp.count(key)){
        deleteNode(key);
        insertAtStart(key,value);
        return;
       }else{
        if(cap == 0){
            // cout<<key<<" ";
            deleteFromTail();
            cap++;
        }
       insertAtStart(key,value);
       cap--;

       }

    }
};

#include <iostream>
using namespace std;

int main() {
    int capacity;
    cout << "Enter cache capacity: ";
    cin >> capacity;

    LRUCache cache(capacity);

    int choice;
    while (true) {
        cout << "\n1. put\n2. get\n0. exit\n";
        cout << "Enter choice: ";
        cin >> choice;

        if (choice == 0) {
            cout << "Exiting...\n";
            break;
        }

        if (choice == 1) {
            int key, value;
            cout << "Enter key and value: ";
            cin >> key >> value;
            cache.put(key, value);
            cout << "Inserted (" << key << ", " << value << ")\n";
        }
        else if (choice == 2) {
            int key;
            cout << "Enter key: ";
            cin >> key;
            int result = cache.get(key);
            if(result == -1) cout<<"Key has not been inserted yet ";
            else
            cout << "Value: " << result << endl;
        }
        else {
            cout << "Invalid choice\n";
        }
    }

    return 0;
}



