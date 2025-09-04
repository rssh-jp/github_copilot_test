package com.example.testapp2.ui.components

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Person
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.example.testapp2.data.User

@Composable
fun UserItem(user: User) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 4.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Icon(
            Icons.Default.Person,
            contentDescription = "ユーザー",
            modifier = Modifier.padding(end = 8.dp)
        )
        Text(
            text = user.name,
            style = MaterialTheme.typography.bodyLarge
        )
        if (user.score > 0) {
            Spacer(modifier = Modifier.weight(1f))
            Text(
                text = "${user.score}点",
                style = MaterialTheme.typography.bodyMedium
            )
        }
    }
}
