package com.example.testapp2.ui.components

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import com.example.testapp2.data.Session
import com.example.testapp2.data.User

@Composable
fun SessionItem(
    session: Session, 
    users: List<User>,
    onClick: () -> Unit = {}
) {
    var expanded by remember { mutableStateOf(false) }
    
    Card(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 8.dp)
            .clickable { onClick() }
    ) {
        Column(
            modifier = Modifier.padding(16.dp)
        ) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text(
                    text = session.name,
                    style = MaterialTheme.typography.titleMedium
                )
                
                Text(
                    text = "参加者: ${users.size}名",
                    style = MaterialTheme.typography.bodyMedium
                )
            }
            
            TextButton(onClick = { expanded = !expanded }) {
                Text(if (expanded) "詳細を隠す" else "詳細を表示")
            }
            
            if (expanded && users.isNotEmpty()) {
                Divider(modifier = Modifier.padding(vertical = 8.dp))
                
                Column {
                    users.forEach { user ->
                        UserItem(user = user)
                    }
                }
            }
            
            Text(
                text = "タップしてセッション詳細を表示",
                style = MaterialTheme.typography.bodySmall,
                modifier = Modifier.align(Alignment.End).padding(top = 8.dp),
                color = MaterialTheme.colorScheme.secondary
            )
        }
    }
}
