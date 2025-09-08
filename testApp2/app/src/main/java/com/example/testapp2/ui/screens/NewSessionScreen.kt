package com.example.testapp2.ui.screens

import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import com.example.testapp2.data.AppState
import com.example.testapp2.ui.theme.TestApp2Theme
import kotlinx.coroutines.launch

@Composable
fun NewSessionScreen(
    modifier: Modifier = Modifier, 
    appState: AppState,
    db: com.example.testapp2.data.db.AppDatabase? = null,
    onSessionCreated: (Int) -> Unit
) {
    var sessionName by remember { mutableStateOf("") }
    val scope = rememberCoroutineScope()

    Column(
        modifier = modifier.padding(16.dp).fillMaxSize(),
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        // セッション作成UI
        Text(
            text = "新しいセッションを作成",
            style = MaterialTheme.typography.headlineMedium,
            modifier = Modifier.padding(bottom = 16.dp)
        )
        
        OutlinedTextField(
            value = sessionName,
            onValueChange = { sessionName = it },
            label = { Text("セッション名") },
            placeholder = { Text("新しいセッション名を入力") },
            modifier = Modifier.fillMaxWidth().padding(vertical = 8.dp)
        )
        
        Button(
            onClick = {
                if (sessionName.isNotBlank()) {
                    val session = appState.addSession(sessionName)
                    if (db != null) {
                        scope.launch {
                            val newId = appState.persistNewSession(db, session)
                            onSessionCreated(newId)
                        }
                    } else {
                        // セッション詳細画面に遷移
                        onSessionCreated(session.id)
                    }
                }
            },
            modifier = Modifier.padding(vertical = 16.dp)
        ) {
            Text("セッションを作成して次へ")
        }
    }
}

@Preview(showBackground = true)
@Composable
fun NewSessionPreview() {
    TestApp2Theme {
        NewSessionScreen(
            appState = AppState(),
            onSessionCreated = {}
        )
    }
}
